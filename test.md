# AMT/NORM end-to-end test

## Setup

- **Sender:** 193.190.248.24 (multicast-capable, gre1 interface to GÉANT backbone)
- **Receiver:** 130.104.229.28 (Rocky Linux, normCast built with NORM_AMT=ON)
- **AMT relay:** `amt-relay.geant.org` (anycast), resolves to 62.40.96.41
- **Group:** 232.1.1.1:7000 (SSM)

## Build

### Sender (193.190.248.24)

No AMT support needed on the sender side.

```bash
mkdir -p build && cd build
cmake .. -DNORM_BUILD_EXAMPLES=ON
make -j$(nproc) normCast
```

### Receiver (130.104.229.28)

```bash
mkdir -p build && cd build
cmake .. -DNORM_BUILD_EXAMPLES=ON -DNORM_AMT=ON
make -j$(nproc) normCast
```

## Run

### Sender

```bash
./normCast send normCast addr 232.1.1.1/7000 id 1 rate 10000 srcaddr 193.190.248.24
```

Sends the `normCast` binary itself as a test payload at 10 kbps.

### Receiver (via AMT)

```bash
./normCast recv /tmp/t addr 232.1.1.1/7000 ssm 193.190.248.24 amt amt-relay.geant.org
```

## Verify

### AMT handshake and data flow (receiver side)

```bash
sudo tcpdump -i eth0 -n -X udp port 2268
```

### Multicast traffic leaving the sender

```bash
sudo tcpdump -vv -i gre1 -n udp port 7000
```

### Multicast routing state on sender (check Oifs is populated after receiver joins)

```bash
sudo ip mroute show table all
```

Expected entry once the AMT relay has sent a PIM-SSM join upstream:

```
(193.190.248.24,232.1.1.1)    Iif: gre1    Oifs: ...    State: resolved
```

If `Oifs` is empty the relay has not built the PIM tree — see **Known issue** below.

## What works

- Full AMT handshake: Discovery → Advertisement → Request → Membership Query → Membership Update
- IGMPv3 SSM join: MODE_IS_INCLUDE, group=232.1.1.1, source=193.190.248.24
- Membership refresh: keep-alive fires every 125 s (relay-advertised QQIC)

## Known issue — no data arrives

The GÉANT relay at 62.40.96.41 accepts the join but sends no AMT Multicast Data packets.
`ip mroute show` on the sender shows an empty `Oifs` for (193.190.248.24, 232.1.1.1),
meaning the relay has not sent a PIM-SSM join upstream for this (S,G).

**Hypothesis:** the relay has no PIM peering path to the 193.190.248.24 prefix, or
232.1.1.1 is not a group the relay is configured to serve.

**Next steps:**
1. Find a known-working GÉANT multicast stream served by `amt-relay.geant.org` and test against that.
2. Ask the GÉANT NOC whether PIM-SSM peering is in place for 193.190.248.24.
3. Run a local AMT relay (`amtrelayd`) to test both ends under local control.
