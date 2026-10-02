# AMT Support in NORM

NORM can receive SSM multicast sessions through an
[AMT](https://www.rfc-editor.org/rfc/rfc7450) (Automatic Multicast Tunneling)
relay when native multicast routing is not available on the path between the
sender and the receiver.  AMT is a receive-only mechanism: only receivers use
it; senders transmit on the native multicast group as usual.

## Building with AMT support

AMT support is disabled by default.  Enable it at CMake configure time:

```
cmake -DNORM_AMT=ON ..
cmake --build .
```

The `libamt` submodule (`libamt/`) must be present (it is registered in
`.gitmodules`).  If it was not cloned with the rest of the repository, run:

```
git submodule update --init libamt
```

## Concepts

| Term | Meaning |
|---|---|
| SSM group | An (`S`, `G`) multicast channel: a specific source address `S` sending to multicast group `G`. |
| AMT relay | A router with native multicast connectivity that accepts AMT tunnels from gateways on the public Internet. |
| AMT gateway | The receiver-side endpoint of the tunnel, implemented inside NORM when `NORM_AMT` is enabled. |

AMT only supports SSM (source-specific multicast).  The NORM session must
therefore be configured with both a multicast group address and the sender's
unicast source address (`ssm` option).

## API

Two calls configure the receiver session before `NormStartReceiver()`:

```c
// Set the SSM source address (the sender's unicast IP)
NormSetSSM(session, "192.0.2.1");

// Set the AMT relay to tunnel through (port defaults to 2268)
NormSetAMTRelay(session, "203.0.113.5");
```

When `NormSetAMTRelay()` is called, NORM opens an AMT gateway tunnel to the
relay instead of joining the multicast group natively.  The rest of the NORM
API is unchanged.

## normCast example

`normCast` is the reference command-line tool for file transfer over NORM.

### Sender

The sender transmits on the SSM group using standard multicast; no AMT
options are needed:

```
normCast send file.dat addr 232.1.2.3/7000 id 1 rate 1000000
```

### Receiver — native SSM (multicast routing available)

```
normCast recv /tmp/rx addr 232.1.2.3/7000 id 2 ssm 192.0.2.1
```

`ssm <sourceAddr>` sets the SSM filter to the sender's IP address.

### Receiver — via AMT (no multicast routing)

```
normCast recv /tmp/rx addr 232.1.2.3/7000 id 2 ssm 192.0.2.1 amt 203.0.113.5
```

`amt <relayAddr>` routes the session through the AMT relay at `203.0.113.5`
(UDP port 2268).  A non-default port can be included: `amt 203.0.113.5:2268`.

### Parameter summary

| Option | Side | Description |
|---|---|---|
| `addr <group>[/<port>]` | both | Multicast group address and port |
| `ssm <sourceAddr>` | receiver | Sender's unicast IP (required for AMT) |
| `amt <relayAddr>[:<port>]` | receiver | AMT relay address; activates tunneling |
| `interface <name>` | both | Network interface to use |

## Limitations

- AMT is **receive-only**.  Senders always use the native multicast network.
- Only SSM channels are supported; ASM (any-source multicast) is not.
- The AMT relay must be reachable over UDP from the receiver's host.
  Some networks block UDP port 2268; check with your network operator.
