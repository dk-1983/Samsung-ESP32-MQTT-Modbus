# Inline UART bridge and 4VRS dispatcher

The ESP32 sits between the Samsung motherboard and the original display/Wi-Fi
board. Factory traffic is forwarded while local web, MQTT and Modbus control
share the 4VRS polling and command dispatcher.

## Wiring

| Connection | ESP32 GPIO | Module pad |
|---|---|---|
| Motherboard TX → ESP32 RX, through level conversion | 18 | 11 |
| ESP32 TX → motherboard RX | 17 | 10 |
| Display CN1 TX → ESP32 RX, through level conversion | 15 | 8 |
| ESP32 TX → display CN1 RX | 16 | 9 |
| Modbus receiver RO → ESP32 RX, through level conversion | 8 | 12 |
| ESP32 TX → Modbus DI | 9 | 17 |
| Modbus DE and /RE | 21 | 23 |

Retain display-board +5 V, common ground and other original connections.
TX/RX labels refer to the respective device. Follow the
[electrical schematic](../hardware/README.md); do not join TX outputs.
Both Samsung UARTs use 9600 8N1. Modbus RTU defaults to 9600 8E1.

## Factory functionality

The bridge is designed to preserve factory functionality, including the display,
IR remote and Samsung SmartThings, while adding local 4VRS interfaces. These
three factory functions have been checked on AR24BSFCMWKNER; every possible
factory feature and other AC models have not been exhaustively validated.
Unknown or damaged factory data is forwarded unchanged. Frame buffering can
introduce latency; the bridge does not guarantee original inter-byte timing.

## Polling and command dispatch

The dispatcher tracks factory transactions and inserts own requests in an idle
window. During an own transaction, factory requests can be held in a bounded
queue; notifications and acknowledgements continue through the bridge.
Local commands require current feedback, control permission and an available
transaction window. Busy commands are rejected, not queued for later execution.

Own replies are separated from factory traffic by envelope, group, response type
and counter. A command is confirmed by own readback of the requested state;
a write acknowledgement alone is not sufficient. No automatic write retries occur.
Factory notifications continue updating the displayed AC state.

Factory initialization and notification acknowledgements remain the responsibility
of the original board. The ESP32 does not duplicate them in inline mode.

## Diagnostics and availability

Use About and `/system/status` for `inline_bridge` and bridge diagnostics:
traffic counts, pending transactions, queue use, parse errors, timeouts and UART
configuration. Three hardware UARTs are used; logs are available through the web.

UART forwarding starts enabled after boot. Disabling UART transmission stops
both local commands and factory forwarding, discarding pending queues. It does
not record traffic for later replay. There is no hardware bypass: loss of ESP32
power or stopped forwarding interrupts communication between the factory boards.

Very late replies after transaction-counter reuse remain a protocol limitation.
Physical RS485 and forced OTA rollback still require validation.
