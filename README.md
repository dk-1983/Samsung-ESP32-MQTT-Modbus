![Samsung-ESP32-MQTT-Modbus — 4VRS](docs/assets/banner-Samsung-ESP32.png)


# Samsung-ESP32-MQTT-Modbus — 0.5.0

[Русский](README_RU.md)

**Keep Samsung's factory features and add 4VRS control.**

Samsung-ESP32-MQTT-Modbus is an ESP32-S3 expansion controller with a transparent
inline UART bridge between the AC motherboard and its original display/Wi-Fi board.
The architecture is designed to preserve the full factory functionality: the
original boards keep communicating, while the IR remote, display and Samsung
SmartThings coexist with the additional 4VRS interfaces.

Our 4VRS polling and command dispatcher runs on the ESP32. It tracks factory
transactions, selects an idle window for each own request and coordinates access
from the web controls, MQTT and Modbus to the shared UART bus. A command is confirmed
only by reading the actual AC state. Unknown factory packets are forwarded unchanged,
so forwarding is not limited to the commands implemented by our firmware.

Additional interfaces:

- **Wi-Fi:** local network connectivity and OTA firmware updates.
- **HTTP Remote:** a local web remote and HTTP API for control over the network.
- **MQTT / Home Assistant Discovery:** automatic device and entity discovery. No custom Samsung/4VRS integration, HACS package or manual entity configuration is required; use the standard MQTT integration with your broker.
- **Modbus RTU / TCP:** RS485 and network automation integration.

Local 4VRS interfaces do not require Samsung's cloud. Factory SmartThings retains
its own connection through the original Wi-Fi module. Display, IR remote and
SmartThings control have been checked with the bridge on AR24BSFCMWKNER;
exhaustive validation of every factory function is still pending.

Repository: [dk-1983/Samsung-ESP32-MQTT-Modbus](https://github.com/dk-1983/Samsung-ESP32-MQTT-Modbus).
See [bridge wiring and limitations](docs/UART_BRIDGE_RU.md).

Home Assistant is optional: connect the controller to your own MQTT broker and automation server. See [topics, commands, states and the complete Discovery-based inventory](docs/HOME_ASSISTANT.md#own-mqtt-broker-without-home-assistant).

[Complete Modbus register list with Samsung / 4VRS separation](docs/MODBUS.md): the factory indoor-unit range ends at **2449**, and our extensions begin at **2450**.

[Electrical schematic, wiring and PCB component libraries](hardware/README.md).

## Controls and connections

The portal on port80 links to controls, MQTT, Modbus and GitHub updates.
The Control link
opens `/control`; advanced ESPHome controls remain on port8080. Both use `admin` and the same password.

- Independent persistent checkboxes for MQTT, Modbus RTU and Modbus TCP; all default off.
- Browser-configured MQTT broker/port/credentials/topic prefix and Home Assistant discovery.
- Browser-configured Modbus unit and RS485 baud rate; RTU defaults9600 8E1, TCP502.
- GitHub automatic-install checkbox, manual check/install, progress, signed manifest,
  board profile, SHA256 validation and rollback.
- Local ESPHome OTA remains available and is coordinated with the GitHub worker.

[Management and releases](docs/MANAGEMENT.md) · [Full device register map](docs/MODBUS.md)
· [Home Assistant MQTT Discovery](docs/HOME_ASSISTANT.md)
· [Modbus register reference (Russian)](docs/MODBUS_REGISTERS_RU.md)
· [UART catalog and test sequence](docs/FUNCTIONS_RU.md)

UART TX/RX GPIO17/18 uses9600 8N1. RS485 TX/RX/DE uses9/8/21.
The AC uses D0 UART, not native Modbus; ESP32 implements the external register map.
Core addresses follow MIM-B19N/B19NT; custom functions begin at2450.

UART is enabled on every boot in 0.4.2 and can be disabled in the control page.
Saving MQTT settings reboots the controller after two seconds to apply credentials.
Feedback is never optimistic: acknowledgement is not proof of the requested AC state.

## Build and validation

Run `./Build.ps1` to install pinned dependencies, run host tests and build, without
flashing. Local configuration is in `secrets.yaml`; neither it nor locally built
private binaries belong in a public release. OTA/factory binaries are under
`work/build/.pioenvs/samsung-s3`. On the first private0.4.0 boot, credentials migrate
to NVS and survive subsequent public OTA updates.

The separate GitHub workflow prepares a clean public **OTA-only** artifact for
already provisioned0.4.0+ devices. Signing/publishing is a separate step.
See [release preparation](docs/MANAGEMENT.md#release).

Power, fan Low/Medium/High/Turbo and both swing axes were tested on the target AC
in earlier firmware. Persistent web settings, Modbus TCP and GitHub OTA from0.4.0
to0.4.1 passed on the device, including credential/settings retention and boot confirmation.
Authenticated MQTT connectivity is verified. Physical RS485 and forced rollback still need testing.
Experimental legacy controls require per-function verification on this model.

[Protocol](docs/PROTOCOL.md) · [Attribution](THIRD_PARTY.md)

The `/about` page shows firmware, memory, connections and UART freshness.
Restart requires confirmation and preserves settings. It is rejected during OTA
or while an AC command is pending.

`/settings` independently changes web, local OTA and Samsung-Setup passwords.
Blank fields preserve current credentials; new values require confirmation.
Changes are stored in NVS and applied on restart. Forgotten-password recovery
is not implemented yet.

`/wifi` shows network, IP, MAC, signal and channel. `/wifi/reset` confirms a
Wi-Fi-only reset; reprovision through Samsung-Setup at `192.168.4.1:8080`.
Other settings are preserved.

[Overview, web controls and MQTT power commands (Russian)](docs/CONTROL_RU.md).

Version 0.4.7 automatically restores UART control permission (register 01) after a reset. HVAC commands wait for readback; requested power is never restored automatically.

In inline mode the factory board owns startup and notification ACKs. Own writes
require fresh permission and feedback, use an idle transaction window and are
confirmed by own readback, never by ACK alone. No automatic write retries.
Main RX18/TX17, factory RX15/TX16, RS485 RX8/TX9/DE21. All three hardware
UARTs are used; serial logging is disabled, web logging remains available.
Disabling UART transmission also stops factory forwarding.
