# V2.0.1 known issues

Boards in this revision were sent to fab with these issues. Fix them in the next revision.

1. **CH05D pin header is mirrored.** The CH05D footprint has its pins in the reverse left-right order from the real module. The footprint came from a provisional library part built from a product photo (`kicad-lib-CH05D`), not from verified dimensions. Workaround: solder the module upside down, with the chip facing the carrier board. Check clearance before soldering. Confirm the actual pin order on a physical module before fixing the footprint.

2. **FLT, DEMP, and FMT are left unconnected.** The GY-PCM5102A breakout's solder jumpers for these pins were open (XSMT was confirmed open by resistance measurement; the others were inferred from behavior), so the pins float. Floating inputs caused crackling and popping on the breadboard. Fix: tie FLT, DEMP, and FMT to GND on the board. XSMT is driven from XIAO GPIO7 (`D8`) in firmware and should be connected to the board's XSMT net.

3. **Analog and digital ground are isolated.** The audio ground (the GY-PCM5102A pad paired with OUTL, which goes to the CH05D `GND` pin) has no connection to the main `GND` net. Add a single star-point connection between the two.

4. **The CH05D's VCC is on the diode-protected rail.** It is fed through the reverse-protection diode, which drops voltage under load and is rated below the amplifier's peak current. Feed VCC from the raw +5 V net. The DAC's VIN can stay on the protected rail or move to raw +5 V.

5. **The XIAO antenna should face away from the analog section.** Keep the antenna keep-out zone free of copper and components, per Seeed's guidance. Breadboard testing showed WiFi coupling into the audio path.

6. **AVDD is unconnected, which is correct.** On the GY-PCM5102A it is an output of an onboard LDO fed from VIN, not an input. Leave it unconnected.
