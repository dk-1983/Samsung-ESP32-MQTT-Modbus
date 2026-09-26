# 4vrs → Samsung-ESP32 — complete device Modbus map 0.3.0

Source: Samsung DB68-07538A-03, pages 8–10 and 13–18.
[Official manual](https://images.samsung.com/is/content/samsung/assets/es/docs/subcarpeta-1/IM-MIM-B19N-DB68-07538A-03_IM_Modbus_Interface_Module_GB_EN_221130-D01.pdf).
Local original: `reference/Samsung-MIM-B19N-B19NT-official-manual.pdf`.

This is the complete map of **our Samsung-ESP32 device**, intended for its own
Home Assistant Modbus Devices definition under **4vrs → Samsung-ESP32**.
MIM-B19N provides the address and value conventions for matching functions;
it does not limit the UART features exposed by our product. The ESP32 translates
Modbus to the existing D0 Wi-Fi UART profile.
All addresses below are decimal **zero-based PDU addresses**. IU0 is fixed; it is
independent of Modbus unit ID (default 1). The factory indoor formula is
`50 + 50*IU + offset`, IU 0–47. Thus IU0 occupies 50–99 and IU47 ends at 2449.
Keep unused factory addresses reserved, including extension slots 82–99 and
factory MessageSet configuration at 6000/7000.

RTU: **9600 8E1**, GPIO9 TX / GPIO8 RX / GPIO21 DE, active high.
TCP: port 502. FC03/04 read; FC06/16 write. Coils and FC01/05/15 are removed (01).
Both read functions expose the same readable words in this implementation.
FC16 accepts up to 8 words, validates the entire request before submitting one
UART command, and rejects mixed core/extended or multiple extended writes (03).
Positive response means accepted, not confirmed: check subsequent feedback.

## Implemented factory positions (IU0)

| PDU | Access | Meaning and values |
|---:|---|---|
| 50 | R | Locally derived communication bits: 0 before feedback, 7 ready, 11 after feedback expires |
| 51 | R | Unit type: FFFF (unknown); raw Wi-Fi model bytes are not a NASA type |
| 52 | R/W | Power: 0 off, 1 on |
| 53 | R/W | Mode: 0 Auto, 1 Cool, 2 Dry, 3 Fan, 4 Heat |
| 54 | R/W | Fan: 0 Auto, 1 Low, 2 Medium, 3 High |
| 55 | R/W | Vertical swing: 0 off, 1 on; preserves horizontal swing |
| 57 | W | Filter reset: 0 no action, 1 submits experimental UART 13/44=00 |
| 58 | R/W | Target ×10; supported UART values 160–300, multiples of 10 |
| 59 | R | Room temperature ×10, signed int16 |

Example: FC06 address 58 value 240 requests 24°C. FC03 address 52 count 4 reads
power/mode/fan/vertical swing. FC04 address 58 count 2 reads both temperatures.
One-based software displays PDU52 as register 53 (often holding reference 40053).
Always distinguish PDU addresses from display notation.

The factory map has no ordinary indoor Turbo fan code: read 54 returns 0B while
Turbo is reported. Use custom 2486 to observe/control it. Out-of-range values in
factory 52–55 are acknowledged and ignored, as documented; they send no UART
command. Fractional or out-of-range targets are rejected (03), not rounded.
Mode changes preserve power. The AC may impose additional mode-specific limits.
Filter reset is implemented from the legacy UART source but has NOT been tested
on this AC; ACK alone cannot prove that the reminder was cleared.

## Reserved manufacturer positions

The product definition uses the actual functions listed here, without placeholder
entities for other Samsung equipment. Unassigned manufacturer addresses stay reserved
for future matching functions. In particular, 69–71 belong to ERV equipment rather
than the room AC Fan mode; 78 is hydro Quiet. Our room AC Quiet uses2487.
The raw error/beep catalog stays available with its original bytes. Use the readable
ranges above: a request crossing an unassigned address fails as a whole (02).

## Custom continuation after the complete factory indoor range

These addresses belong to **our project**, not Samsung. No compatibility aliases for other devices are provided.

| PDU | Access | Meaning |
|---:|---|---|
| 2450–2474 | R / selected W | Raw one-byte UART catalog: address = 2450 + index |
| 2475 | R | Last core status age in seconds; 65535 if never received |
| 2476–2477 | R | Core status frame count, low/high words |
| 2478 | R | Core command result: 0 idle, 1 pending, 2 confirmed, 3 timeout, 4 cancelled |
| 2479 | R | Flags: bit0 core fresh, bit1 RTU configured, bit2 TCP configured, bit3 TX enabled |
| 2480 | R | UART TX enabled; cannot be changed through Modbus |
| 2481 | R | Extended result: same codes, plus 5 acknowledged/unverified action |
| 2482 | R | Last extended catalog index; 255 if none |
| 2483 | R | Catalog size (25) |
| 2484 | R/W | Swing: 0 off, 1 vertical, 2 horizontal, 3 both |
| 2485 | R/W | Preset: 0 none, 1 boost, 2 sleep, 3 quiet, 4 legacy smart, 5 soft cool, 6/7/8 legacy wind |
| 2486 | R/W | Full UART fan: 1 Low, 2 Medium, 3 High, 4 Auto, 5 Turbo |
| 2487 | R/W | Quiet: 0 off, 1 on; turning off preserves other active presets |
| 2488–2499 | — | Reserved by this project |
| 2500–2949 | R | Raw catalog blocks: `2500 + 18*index`; length, 16 packed words, age |

See [UART catalog and write allowlist](FUNCTIONS_RU.md). Both read functions may
read custom fields; write-only actions are not exposed as a claimed persistent state.
Raw catalog bytes retain their literal meaning, including FE/FC/FF responses.

## Migration and product behavior

This release changes the external contract; it is not backward compatible with 0.2.0:
coil0 → holding52; target0 → 58 with ×10; mode1 → 53 with Samsung values;
fan2 → 54 (Turbo uses2486); swing4 →2484; preset5 →2485; quiet coil1 →2487.
Raw holding100+index →2450+index; input4–12 →2475–2483;
raw input200+18*index →2500+18*index. RTU changes from19200 8N1 to9600 8E1.
Update controller configuration before installing the firmware.

Unlike the factory adapter's retained cache, unknown/stale feedback returns0B;
we do not publish a plausible temperature, mode or error code without evidence.
Per-field read expiry is30s; writes require fresh core and affected fields within10s.
Communication word50 is derived from UART feedback, not actual NASA tracking.
Firmware0.4.2 enables UART after every reboot at the owner's request; older versions
started monitor-only. Exceptions:01 unsupported FC,02 address,
03 value/batch,06 busy or TX disabled,0B unknown/stale/unrepresentable state.
Repeated writes are commands, not a way to poll. Keep requests at least10ms apart.

Host tests cover factory conversions, signed scaling, swing-axis preservation,
ignored values, atomic writes, unsupported functions, extension isolation and the
existing UART confirmation/freshness/parser tests. A successful build does not
constitute live testing of the new map, physical RS485 or an external MIM integration.

## Runtime settings in0.4.0

MQTT, RTU and TCP now have independent persistent checkboxes on the portal. RTU and TCP
default off on first0.4.0 startup. Unit/baud are configured in the browser; parity stays Even.
Diagnostic2479 bits1/2 reflect enabled transports. Register addresses remain unchanged.

## Register reference

[Register information sheet in Russian](MODBUS_REGISTERS_RU.md):
addresses, values, write confirmation, raw catalog and polling behavior.
