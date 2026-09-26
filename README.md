![Samsung-ESP32-MQTT-Modbus — 4VRS](docs/assets/banner-Samsung-ESP32.png)


# Samsung-ESP32-MQTT-Modbus — 0.4.7

[Русский](README_RU.md)

Local Samsung air conditioner control through ESP32-S3-WROOM-1 N16R8 and the
stock Wi-Fi module's UART. Target AC: AR24BSFCMWKNER. No Samsung cloud required.
Repository: [dk-1983/Samsung-ESP32-MQTT-Modbus](https://github.com/dk-1983/Samsung-ESP32-MQTT-Modbus).

The separate experimental [UART bridge build](docs/UART_BRIDGE_RU.md) forwards
factory traffic in both directions. It requires different wiring; command
injection, MQTT and Modbus are not included in this diagnostic build.

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
