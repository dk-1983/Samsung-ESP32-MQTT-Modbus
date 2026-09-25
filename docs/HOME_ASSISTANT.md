# Home Assistant through MQTT Discovery

[Русский](HOME_ASSISTANT_RU.md)

Samsung 0.4.1 already publishes MQTT Discovery. As in our Haier project, enable
MQTT and discovery on the controller. No custom Samsung integration, Hub, HACS
package or manually configured YAML entities are required.

1. Add the standard MQTT integration in Home Assistant under Settings → Devices
   & services. Configure your MQTT broker.
2. Open `http://<controller-IP>/mqtt`; enter the same broker, port (usually 1883),
   username and password. Enable MQTT and Home Assistant Discovery and save.
   A blank password preserves the saved value.
3. Wait for a successful broker connection. **Samsung UART Prototype** should
   appear under MQTT with **Air conditioner**, settings, buttons and diagnostics
   matching the firmware entities.
4. Every boot disables UART transmission. Explicitly enable **UART transmission
   experimental** in the web controls when ready to operate the AC. Check
   **AC feedback fresh** for current AC feedback.

If no device appears, check broker authentication, matching broker addresses,
publish/subscribe permissions and HA discovery prefix `homeassistant`.
Saving settings does not prove that authentication succeeded.

## Relationship to Haier

Persistent settings, a separate discovery checkbox, retained configuration and
grouping entities into one device follow Haier's approach. Samsung uses ESPHome's
MQTT publisher; it does not copy Haier's custom MQTT worker or `Discovery.h` verbatim.

- Configuration topics: `homeassistant/<type>/samsung-s3/<entity>/config`.
  Entity unique IDs use the controller MAC address.
- Operational topics use the configured prefix (default `samsung-s3`). Read actual
  command/state topics from discovery; Haier `/set/<field>` commands do not apply.
- Retained discovery is delivered when HA reconnects. Samsung 0.4.1 does not have
  Haier's explicit `homeassistant/status` birth handler.
- MQTT availability represents controller connectivity. **AC feedback fresh**
  separately reports UART freshness; Haier's combined availability is not implemented.
- Disabling discovery does not delete configurations already retained by the broker.

Experimental entities still require hardware validation. Requested AC values are
not treated as received feedback. GitHub OTA 0.4.0 → 0.4.1 passed on the bench;
end-to-end HA discovery awaits valid broker credentials after anonymous access
was rejected.

Reference: [Home Assistant MQTT documentation](https://www.home-assistant.io/integrations/mqtt/).
