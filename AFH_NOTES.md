# AFH development notes

## Goal
Adaptive Frequency Hopping for SlimeNRF ESB link.

## Integration point
Radio layer: src/connection/esb.c

## Current stage
- AFH manager added.
- Next: integrate channel switching with ESB.
- Next: receiver synchronization protocol.

## Rules
Keep pairing and packet format compatibility until AFH negotiation exists.
