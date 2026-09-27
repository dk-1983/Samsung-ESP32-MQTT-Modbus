#pragma once
#include <cstdint>
namespace samsung_management {
// A failed publish leaves the event pending. Readiness must remain stable for 5 s.
class BootEvents {
 public:
 enum Event { NONE, BOOT_READY, MQTT_RECONNECTED, AC_CONNECTION_RESTORED };
 Event poll(uint32_t now, bool connected, bool healthy, bool ac_ready) {
  if (!connected) { connected_ = false; stable_ = false; return NONE; }
  if (!connected_) { connected_ = true; reconnect_ = sent_; }
  if (!healthy) { stable_ = false; return NONE; }
  if (!stable_) { stable_ = true; since_ = now; }
  if (uint32_t(now - since_) < 5000) return NONE;
  if (!sent_) return BOOT_READY;
  if (reconnect_) return MQTT_RECONNECTED;
  if (!ac_ready) { ac_seen_ = false; ac_stable_ = false; }
  else if (!ac_seen_) {
   if (!ac_stable_) { ac_stable_ = true; ac_since_ = now; }
   if (uint32_t(now - ac_since_) >= 5000) return AC_CONNECTION_RESTORED;
  }
  return NONE;
 }
 void published(Event event, bool ac_ready) {
  if (event == NONE) return;
  sent_ = true; reconnect_ = false; ac_seen_ = ac_ready; ac_stable_ = false;
 }
 static const char *name(Event event) {
  switch (event) {
   case BOOT_READY: return "boot_ready";
   case MQTT_RECONNECTED: return "mqtt_reconnected";
   case AC_CONNECTION_RESTORED: return "ac_connection_restored";
   default: return "none";
  }
 }
 private:
 bool sent_=false, connected_=false, reconnect_=false, stable_=false;
 bool ac_seen_=false, ac_stable_=false;
 uint32_t since_=0, ac_since_=0;
};
}
