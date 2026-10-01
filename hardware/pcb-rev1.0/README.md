# Samsung UART bridge — PCB rev1.0

[Русский](README_RU.md)

30 × 48 mm (48 × 30 when rotated), two copper layers, 1.0 mm thickness in KiCad. Components on both sides, no mounting holes. Based on the Haier pcb-v1.1 circuit; the Haier project was not modified.

Files: samsung-rev1.0.kicad_pcb; review/samsung-both-sides.png (review only); circuit.json, connections.csv and BOM.csv; drc.json; review/pin-check.json; reproducible scripts in sources/.

KiCad DRC: 0 violations and 0 unconnected items. All 82 assigned logical pads match circuit.json. 25 U1/U2/U3 assignments independently match the Samsung schematic. This PCB has not been tested on hardware. Gerber and drill files are included in production/, together with an archive and a checksum manifest.

## Solder pads

MAIN and DISPLAY TX/RX labels refer to the external Samsung board, not the ESP.

- TP9–TP12 MAIN, left to right viewed from top: +5V, GND, motherboard TX through divider to GPIO18, motherboard RX from GPIO17.
- TP13–TP16 DISPLAY: +5V, GND, display TX through divider to GPIO15, display RX from GPIO16.
- TP7/TP8: RS485 A/B.
- TP1/TP2/TP3: programming TX0 GPIO43 / RX0 GPIO44 / GND.
- TP4/TP5/TP6 on bottom: EN / BOOT GPIO0 / 3V3 voltage monitor.

UART solder pads are 2 mm in diameter. No UART connectors. All power and grounds are common. Preserve factory Samsung connections outside the UART bridge.

RS485 uses GPIO8 RX, GPIO9 TX, GPIO21 DE and /RE. R6 10k pulls direction down. R7 120 ohm and solder jumper JP1 enable termination. All three RX inputs use 10k/20k dividers; ESP TX outputs are direct 3.3V.

## Assembly and references

PCB component numbering follows the Haier carrier and differs from the older Samsung revA PDF. Use this PCB's BOM.csv and connections.csv. Old Samsung resistor → PCB mapping: R1→R1, R2→R9, R3→R8, R4→R2, R5→R3, R6→R5, R7→R4, R8→R6. PCB R7 adds termination.

The Haier power circuit is retained: LM1117-3.3 SOT-223, 220uF and 100uF case-C capacitors and ceramic decoupling. This expands the old Samsung revA drawing, whose 47uF C2 package was unresolved. The regulator TAB and its thermal planes are +3V3, not GND. ESP ground planes and thermal vias remain separate. Both copper layers are clear beneath the antenna.

This PCB requires factory plated holes, unlike the homemade kitchen-hood PCB. Verify mechanical fit and the BOM against available components before ordering. No firmware, device or release changes were performed.

