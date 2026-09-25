# Experimental Wi-Fi UART D0 profile

Target electrical layer: direct Wi-Fi-module UART, configured **9600 8N1**.
This is distinct from NASA RS485 **9600 8E1** (32…34). No automatic protocol detection/transmit fallback.
Core control verified on AR24BSFCMWKNER; electrical levels/isolation remain unresolved.

Frame: `D0 C0 02 LEN 00 00 00 00 00 COUNTER FE TYPE_H TYPE_L PAYLOAD_LEN [ID LEN VALUE...] XOR E0`.
Total length = LEN+4 = PAYLOAD_LEN+16. LEN is one byte, so payload maximum 243 bytes.
Checksum is XOR of all bytes before checksum/footer. Lengths, header, footer, checksum and TLV boundaries
are validated before interpretation. Parser is bounded to 259 bytes; incomplete frames expire after 200 ms.
Unknown register IDs are skipped, unknown enum values invalidate that field instead of guessing.

Types: 1202 read, 1203 read response, 1204 write, 1205 write response, 1206 notification, 1207 notification ACK.
Only 1203/1206 update state. Notifications receive ACK only in active mode, with the received counter and payload.
ACK counter semantics still need confirmation on hardware. No automatic retries; max 4 queued ACKs.
Minimum TX interval 300 ms, minimum RX quiet interval 30 ms, status query every 5 s.
Optional explicit initialization is 1204 with `01 01 0F 74 01 F0`; it does not run automatically.

| ID | Meaning | Known values |
|---|---|---|
| 02 | Power | 0F on / F0 off |
| 43 | Mode | 12 cool / 22 dry / 32 fan / 42 heat / E2 auto |
| 5A | Target | whole °C, prototype permits 16–30 |
| 5C | Room | signed whole °C |
| 62 | Fan | 00 auto / 12 low / 14 medium / 16 high / 18 turbo |
| 63 | Swing | C2 stop (verified target); 12 legacy stop read / 92 vertical / A2 horizontal / B2 both |
| 44 | Preset | 12 none / 22 boost / 42 sleep / 52 quiet |

Version 0.2.0 adds legacy presets 32/62/82/92/A2 and an explicit experimental direction selector; see FUNCTIONS_RU.md.
No WindFree assumption. F7 error payload is preserved as raw Samsung bytes.

The first valid state can be collected passively. A command requires fresh core data (power, mode,
temperature and fan) and fresh data for every requested field. Confirmation requires all requested fields
to be observed matching after the transmission; an ACK or unrelated notification cannot confirm a command.
This is observed feedback, not a proven transaction identifier match. Hardware may require additional
handshake/registration messages, a different checksum/profile, or different register enums.

Sources:
- [kumy/esphome_samsung_ac](https://github.com/kumy/esphome_samsung_ac), inspected commit
  `2b7c14002c7fe0b2d8bb24f4c8d245fe6888bc5b`, including protocol tests.
- [Foxhill67/esphome_samsung_ac](https://github.com/Foxhill67/esphome_samsung_ac), different bus family;
  not used as the direct Wi-Fi UART driver.

The checksum condition and unchecked parsing in the referenced Wi-Fi implementation were not reused.
This prototype uses strict equality and bounded parsing, with independent regression tests.

## Extended profile 0.2.0

`extended.h` contains a bounded allowlist of 25 register definitions from kumy/samsung-ac,
commit `7d9df9d8010fe11a8171ebbab044a74d56ee441d`. Groups 12/13/14 share read/write/notify
suffixes 02/03/04/05/06/07. Read responses/notifications alone update raw state.
One pending command is shared across core and extended interfaces; a rejected write has no UART side effects.
Extended ordinary writes confirm only matching subsequent feedback; action ACKs are explicitly unverified.
Raw values expire after 30 seconds; no guessed physical units or universal sentinel meanings.
The observed FC envelope is structurally validated and counted separately; never interpreted as FE climate
feedback and never acknowledged without a protocol reference. See FUNCTIONS_RU.md for the exact catalogue.
