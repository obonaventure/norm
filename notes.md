# Session notes

## AMT end-to-end test — status as of 2026-10-02

### What works

- Build: `cmake -DNORM_BUILD_EXAMPLES=ON -DNORM_AMT=ON ..` compiles cleanly on Rocky Linux.
- AMT handshake: normCast receiver successfully completes the full AMT handshake
  (Discovery → Advertisement → Request → Membership Query → Membership Update) against
  the GÉANT relay.  Verified by tcpdump: the relay responds to every step.
- IGMPv3 SSM join: the Membership Update carries a correctly formed IP-encapsulated
  IGMPv3 report with MODE_IS_INCLUDE, group=232.1.1.1, source=193.190.248.24.
  Confirmed by decoding the raw bytes in tcpdump -X output.
- Membership refresh: the 10 ms timer fires, calls `amt_gateway_timeout()` when the
  relay-advertised query interval (QQIC=125 s) expires, and the relay responds with
  a new Membership Query.  The keep-alive cycle runs correctly.

### Test setup

- Receiver: Rocky Linux machine at 130.104.229.28, normCast built with NORM_AMT=ON.
- Sender: machine at 193.190.248.24 (GRE tunnel `gre1` to GÉANT backbone).
- AMT relay: anycast `amt-relay.geant.org` → resolved to 62.40.96.41 at time of test;
  the Advertisement redirects to 62.40.96.41 (unicast relay endpoint).

### Command lines

**Build (Rocky Linux, from norm/build/):**
```
cmake .. -DNORM_BUILD_EXAMPLES=ON -DNORM_AMT=ON
make -j$(nproc) normCast
```

**Sender** (run on 193.190.248.24, or locally if that address is on gre1):
```
./normCast send normCast addr 232.1.1.1/7000 id 1 rate 10000 srcaddr 193.190.248.24
```

**Receiver via AMT** (run on 130.104.229.28):
```
./normCast recv /tmp/t addr 232.1.1.1/7000 ssm 193.190.248.24 amt amt-relay.geant.org
```

**Verify AMT handshake and data flow:**
```
sudo tcpdump -i eth0 -n -X udp port 2268
```

**Check multicast routing state on source machine:**
```
sudo ip mroute show table all
sudo tcpdump -vv -i gre1 -n udp port 7000
```

### Problem: no multicast data arrives via AMT

The relay accepts the join but never sends any AMT Multicast Data packets to the
receiver.  `sudo ip mroute show table all` on the source machine shows:

```
(193.190.248.24,232.1.1.1)    Iif: gre1    State: resolved    Table: default
```

No `Oifs` entry — meaning no downstream PIM interface has joined the tree.  The AMT
relay at 62.40.96.41 does not appear to have sent a PIM-SSM join upstream for
(193.190.248.24, 232.1.1.1).

### Hypothesis

The GÉANT AMT relay may not have a PIM peering path to the 193.190.248.24 multicast
domain, or 232.1.1.1 is not a group the relay is configured to serve.  The relay
accepts the AMT handshake regardless of whether it can actually forward the stream.

### Next steps to try

1. Find a known-working GÉANT multicast stream (group + source) that the relay at
   `amt-relay.geant.org` is known to serve, and test normCast against that to confirm
   the receive path works end-to-end.
2. Ask the GÉANT NOC / multicast team whether (193.190.248.24, 232.1.1.1) is
   reachable from their AMT relay, and whether PIM-SSM peering is in place for the
   193.190.248.24 prefix.
3. On the relay side (if accessible): check `show ip mroute 232.1.1.1` to see whether
   the relay has a forwarding entry for this (S,G).
4. Alternative: test with a group in the 239.x.x.x range (ASM) that GÉANT is known
   to carry — though note that libamt and normCast currently only support SSM joins.
