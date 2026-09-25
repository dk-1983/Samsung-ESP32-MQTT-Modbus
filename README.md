![Samsung-ESP32-MQTT-Modbus — 4VRS](docs/assets/banner-Samsung-ESP32.png)

> Version 0.4.2: UART is enabled on every boot at the owner's request; any boot monitor-only behavior described below applies to 0.4.1. Saving MQTT settings reboots after two seconds to recreate the connection.


# Samsung-ESP32-MQTT-Modbus — 0.4.2

[Русский](README_RU.md)

Local Samsung air conditioner control through ESP32-S3-WROOM-1 N16R8 and the
stock Wi-Fi module's UART. Target AC: AR24BSFCMWKNER. No Samsung cloud required.
Repository: [dk-1983/Samsung-ESP32-MQTT-Modbus](https://github.com/dk-1983/Samsung-ESP32-MQTT-Modbus).

## Controls and connections

The portal on port80 links to controls, MQTT, Modbus and GitHub updates.
Settings pages reuse the design and behavior of our Haier project. The Control link
opens the existing ESPHome controls on port8080. Both use `admin` and the same password.

- Independent persistent checkboxes for MQTT, Modbus RTU and Modbus TCP; all default off.
- Browser-configured MQTT broker/port/credentials/topic prefix and Home Assistant discovery.
- Browser-configured Modbus unit and RS485 baud rate; RTU defaults9600 8E1, TCP502.
- GitHub automatic-install checkbox, manual check/install, progress, signed manifest,
  board profile, SHA256 validation and rollback inherited from Haier.
- Local ESPHome OTA remains available and is coordinated with the GitHub worker.

[Management and releases](docs/MANAGEMENT.md) · [Full device register map](docs/MODBUS.md)
· [Home Assistant MQTT Discovery](docs/HOME_ASSISTANT.md)
· [Modbus Devices implementation handoff (Russian)](docs/MODBUS_DEVICES_HANDOFF_RU.md)
· [UART catalog and test sequence](docs/FUNCTIONS_RU.md)

UART TX/RX GPIO17/18 uses9600 8N1. RS485 TX/RX/DE uses15/16/21.
The AC uses D0 UART, not native Modbus; ESP32 implements the external register map.
Core addresses follow MIM-B19N/B19NT; custom functions begin at2450.
The Haier project remains unchanged.

Every boot starts in UART MONITOR ONLY. Enabling a network transport does not enable
UART transmission. Explicitly enable UART in the control page. Feedback is never
optimistic: write acknowledgement is not proof of the requested AC state.

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
HA discovery with an authenticated broker, physical RS485 and forced rollback still need testing.
Experimental legacy controls require per-function verification on this model.

[Protocol](docs/PROTOCOL.md) · [Attribution](THIRD_PARTY.md)
