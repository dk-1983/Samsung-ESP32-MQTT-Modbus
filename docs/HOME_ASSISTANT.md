# Home Assistant through MQTT Discovery

[Русский](HOME_ASSISTANT_RU.md)

Samsung-ESP32 publishes MQTT Discovery through ESPHome. No custom Samsung integration,
Hub, HACS package or manually configured YAML entities are required.

## Setup

1. Add the standard MQTT integration under Home Assistant Settings → Devices & services.
2. Open `http://<controller-IP>/mqtt`. Enter the same broker, port (usually 1883),
   username and password. Enable MQTT and Home Assistant Discovery.
3. Save. Firmware 0.4.2 reboots after two seconds to apply the connection settings.
   An empty password preserves the saved value; a separate checkbox clears it.
   The saved password is never returned to the browser.
4. Wait for connection. **Samsung UART Prototype** appears under MQTT with
   **Air conditioner**, settings and diagnostic entities.
5. UART starts enabled on every 0.4.2 boot. The **UART transmission experimental**
   switch in web controls can disable it. **AC feedback fresh** reports UART freshness.

If discovery fails, check authentication, matching broker addresses, publish/subscribe
permissions and the `homeassistant` discovery prefix. Saving does not prove connection.

## Topics and availability

- Discovery: `homeassistant/<type>/samsung-s3/<entity>/config`. Unique IDs use the
  controller MAC address; entities share a device entry.
- Operational prefix defaults to `samsung-s3` and is configurable. Read individual
  command and state topics from the discovery payloads.
- Retained configurations are delivered when HA connects. There is no separate
  `homeassistant/status` handler in the current firmware.
- MQTT availability represents controller connectivity. **AC feedback fresh**
  separately reports UART freshness; broker connection does not prove AC connectivity.
- Disabling discovery does not remove configurations already retained by the broker.

Authenticated MQTT, reconnection after reboot and incoming data have been checked
on the bench. Experimental functions still require hardware validation. Requested
values are not published as observed AC feedback.

[Home Assistant MQTT documentation](https://www.home-assistant.io/integrations/mqtt/).

## Own MQTT broker without Home Assistant

Connect the controller to an independent MQTT broker using `/mqtt`: configure the host,
port, credentials and topic prefix. Home Assistant is not required; your own server,
Node-RED or another automation client can publish commands and consume feedback.

MQTT uses topics and message payloads, not Modbus register addresses. To obtain the
interface inventory for the installed firmware, enable MQTT and Discovery, subscribe
to `homeassistant/#` and identify the device configurations by MAC / `device`.
Discovery publishing does not require a running Home Assistant instance. Read command
and state topics, supported values, templates and availability from the configuration
JSON; field names may be abbreviated. Subscribe to `<prefix>/#` to observe operational
messages (default `samsung-s3/#`). Use the actual topics advertised by Discovery.

These retained configurations form a machine-readable entity inventory. Discovery
can be disabled after setting up your client while operational MQTT remains enabled;
disabling Discovery does not erase previously retained configurations.

| Action | Topic with default prefix | Payload |
|---|---|---|
| Power on | `samsung-s3/button/ac_power_on/command` | `PRESS` |
| Power off | `samsung-s3/button/ac_power_off/command` | `PRESS` |

Send commands without retain and confirm execution from AC feedback. Publishing a
message or receiving a broker acknowledgement does not prove AC execution.

[Power commands and HTTP API (Russian)](CONTROL_RU.md) ·
[UART function catalog (Russian)](FUNCTIONS_RU.md) · [Separate Modbus register map](MODBUS.md).
