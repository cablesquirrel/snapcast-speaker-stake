# V2.0.1 known issues

## Open

1. **CH05D pin header is mirrored.** The CH05D footprint has its pins in the reverse left-right order from the real module. The footprint came from a provisional library part built from a product photo (`kicad-lib-CH05D`), not from verified dimensions. Workaround for the boards already built: solder the module upside down, with the chip facing the carrier board. Check clearance before soldering. Confirm the actual pin order on a physical module before fixing the footprint in the next revision.

## Resolved in the final design

- **FLT, DEMP, and FMT floating.** These pins were left unconnected on the breadboard, where they caused crackling and popping. The final design ties them to GND.
- **CH05D VCC on the diode-protected rail.** The amplifier's VCC was fed through the reverse-protection diode, which drops voltage under load. The final design feeds it from the raw +5 V net.

> **Note:** AVDD on the GY-PCM5102A header is unconnected on purpose. It is an output of the module's onboard LDO, fed from VIN, not an input. Leave it unconnected.
