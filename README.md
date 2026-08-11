# SlimeVR Tracker nRF AFH — Quoeskeni fork

Firmware for Nordic nRF52 / nRF54L tracker boards, with AFH support for StackedSmol trackers.

This repository is the tracker-side fork used by `Quoeskeni/SmolSlimeConfigurator-AFH` and `Quoeskeni/SlimeNRF-Firmware-CI`. Upstream/original repositories are read-only context; changes here are intended only for this Quoeskeni fork.

## AFH pairing contract

The configurator's safe **Pair AFH** button sends this sequence to a tracker:

```text
afh_set_channel 100
afh_info
pair
```

The destructive recovery button sends:

```text
clear
afh_set_channel 100
afh_info
pair
```

Tracker firmware therefore guarantees:

- `afh_set_channel 100` moves the runtime ESB channel back to the AFH discovery/pairing channel.
- `pair` entered from USB console starts the pairing loop immediately, even while USB HID/CDC is connected.
- During pairing, the tracker uses the shared discovery ESB address and AFH default channel `100`.
- Serial logs include `Pairing state: started`, `Pairing request received`, `Paired`, `Pairing state: paired`, `Tracker ID`, and `Receiver address` lines for configurator diagnostics.

## SlimeVR server compatibility

No SlimeVR server patch is required. AFH negotiation is radio/firmware-side; the receiver remains a standard HID dongle to the server.

## Hardware

- https://github.com/SlimeVR/SlimeVR-Tracker-nRF-PCB
- https://oshwlab.com/sctanf/slimenrf3

## License

Unless otherwise specified, all code in this repository is dual-licensed under either:

- MIT License ([LICENSE-MIT](LICENSE-MIT) or https://opensource.org/license/mit/)
- Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE) or https://opensource.org/license/apache-2-0/)

at your option.
