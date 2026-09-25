// SPDX-License-Identifier: GPL-3.0-or-later
#include "samsung_portal.h"
#include "WifiPage.h"
#include "esphome/components/wifi/wifi_component.h"
#include <esp_mac.h>
#include <nvs.h>
#include <cJSON.h>
namespace esphome::samsung_portal {
String Portal::wifi_json_(){
 auto *w=wifi::global_wifi_component;cJSON *j=cJSON_CreateObject();if(!j)return "{}";
 bool connected=w&&w->is_connected();char ip[network::IP_ADDRESS_BUFFER_SIZE]="",ssid[33]="",mac_text[18]="",ap_ip[network::IP_ADDRESS_BUFFER_SIZE]="";
 uint8_t mac[6]{};if(esp_read_mac(mac,ESP_MAC_WIFI_STA)==ESP_OK)snprintf(mac_text,sizeof(mac_text),"%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
 if(w){if(connected){w->get_ip_addresses()[0].str_to(ip);w->wifi_ssid_to(ssid);}w->wifi_soft_ap_ip().str_to(ap_ip);}
 cJSON_AddBoolToObject(j,"connected",connected);cJSON_AddStringToObject(j,"ssid",ssid);
 cJSON_AddStringToObject(j,"ip",ip);cJSON_AddStringToObject(j,"mac",mac_text);
 if(connected){cJSON_AddNumberToObject(j,"rssi",w->wifi_rssi());cJSON_AddNumberToObject(j,"channel",w->get_wifi_channel());}
 else{cJSON_AddNullToObject(j,"rssi");cJSON_AddNullToObject(j,"channel");}
 cJSON_AddBoolToObject(j,"ap_active",w&&w->is_ap_active());
 cJSON_AddStringToObject(j,"ap_ssid",w?w->get_ap().get_ssid().c_str():"");
 cJSON_AddStringToObject(j,"ap_ip",ap_ip);
 char *raw=cJSON_PrintUnformatted(j);String result=raw?raw:"{}";if(raw)cJSON_free(raw);cJSON_Delete(j);return result;
}
void Portal::wifi_web_(){
 web_.on("/wifi",HTTP_GET,[this](){if(test_auth_())send_page_(WIFI_PAGE);});
 web_.on("/wifi/status",HTTP_GET,[this](){if(!test_auth_())return;web_.sendHeader("Cache-Control","no-store");web_.send(200,"application/json",wifi_json_());});
 web_.on("/wifi/reset",HTTP_GET,[this](){if(test_auth_())send_page_(WIFI_RESET_PAGE);});
 web_.on("/wifi/reset",HTTP_POST,[this](){
  if(!post_auth_())return;
  if(web_.arg("confirm")!="RESET_WIFI"){web_.send(400,"text/plain","WiFi reset confirmation required");return;}
  if(ac_->session.pending||ac_->extended.pending){web_.send(409,"text/plain","AC command pending; try again shortly");return;}
  // Pinned ESPHome WiFiComponent::start uses key 88491487 for builds without
  // compiled station networks. __init__.py validates this configuration.
  // Flush pending preference writes before removing ONLY the Wi-Fi blob.
  if(!global_preferences->sync()){web_.send(503,"text/plain","Could not sync settings");return;}
  nvs_handle_t handle;
  if(nvs_open("esphome",NVS_READWRITE,&handle)!=ESP_OK){web_.send(503,"text/plain","WiFi storage unavailable");return;}
  esp_err_t err=nvs_erase_key(handle,"88491487");
  if(err==ESP_OK||err==ESP_ERR_NVS_NOT_FOUND)err=nvs_commit(handle);
  nvs_close(handle);
  if(err!=ESP_OK){web_.send(503,"text/plain","Could not clear WiFi network");return;}
  restart_=true;restart_at_=millis()+2000;
  web_.sendHeader("Cache-Control","no-store");web_.send(202,"application/json","{\"reset\":true,\"restarting\":true}");
 });
}
}
