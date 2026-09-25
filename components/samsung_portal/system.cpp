// SPDX-License-Identifier: GPL-3.0-or-later
#include "samsung_portal.h"
#include "AboutPage.h"
#include "version.h"
#include "esphome/components/wifi/wifi_component.h"
#include "esphome/core/application.h"
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <esp_system.h>
#include <esp_mac.h>
#include <cJSON.h>

namespace esphome::samsung_portal {
String Portal::system_json_(){
 cJSON *j=cJSON_CreateObject();if(!j)return "{}";
 auto *w=wifi::global_wifi_component;bool connected=w&&w->is_connected();
 char ip[network::IP_ADDRESS_BUFFER_SIZE]="";
 if(w)(connected?w->get_ip_addresses()[0]:w->wifi_soft_ap_ip()).str_to(ip);
 cJSON_AddStringToObject(j,"version",SAMSUNG_FIRMWARE_VERSION);
 cJSON_AddStringToObject(j,"hostname",App.get_name().c_str());
 cJSON_AddStringToObject(j,"controller","ESP32-S3 · N16R8");
 cJSON_AddStringToObject(j,"ip",ip);
 uint8_t mac[6]{};char mac_text[18]="";
 if(esp_read_mac(mac,ESP_MAC_WIFI_STA)==ESP_OK)
  snprintf(mac_text,sizeof(mac_text),"%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
 cJSON_AddStringToObject(j,"mac",mac_text);
 cJSON_AddNumberToObject(j,"boot_id",boot_id_);
 cJSON_AddNumberToObject(j,"uptime_s",esp_timer_get_time()/1000000);
 cJSON_AddNumberToObject(j,"reset_reason",esp_reset_reason());
 const uint32_t caps=MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT;
 cJSON_AddNumberToObject(j,"free_heap",heap_caps_get_free_size(caps));
 cJSON_AddNumberToObject(j,"min_heap",heap_caps_get_minimum_free_size(caps));
 cJSON_AddNumberToObject(j,"max_block",heap_caps_get_largest_free_block(caps));
 cJSON_AddNumberToObject(j,"psram_size",ESP.getPsramSize());
 cJSON_AddNumberToObject(j,"free_psram",ESP.getFreePsram());
 cJSON_AddNumberToObject(j,"flash_size",ESP.getFlashChipSize());
 cJSON_AddBoolToObject(j,"wifi",connected);
 cJSON_AddBoolToObject(j,"ap",w&&w->is_ap_active());
 cJSON_AddBoolToObject(j,"mqtt_enabled",config_.mqtt);
 cJSON_AddBoolToObject(j,"mqtt_connected",mqtt::global_mqtt_client->is_connected());
 cJSON_AddBoolToObject(j,"rtu",config_.rtu);cJSON_AddBoolToObject(j,"tcp",config_.tcp);
 cJSON_AddBoolToObject(j,"uart",ac_->tx_enabled());
 cJSON_AddBoolToObject(j,"ac_fresh",ac_->feedback_fresh());
 cJSON_AddBoolToObject(j,"storage_ok",storage_ok_);
 cJSON_AddBoolToObject(j,"update_busy",updates_busy_());
 cJSON_AddBoolToObject(j,"restarting",restart_);
 char *raw=cJSON_PrintUnformatted(j);String result=raw?raw:"{}";
 if(raw)cJSON_free(raw);cJSON_Delete(j);return result;
}
void Portal::system_web_(){
 web_.on("/about",HTTP_GET,[this](){if(test_auth_())send_page_(ABOUT_PAGE);});
 web_.on("/system/status",HTTP_GET,[this](){
  if(!test_auth_())return;web_.sendHeader("Cache-Control","no-store");
  web_.send(200,"application/json",system_json_());
 });
 web_.on("/system/restart",HTTP_POST,[this](){
  if(!post_auth_())return;
  if(web_.arg("confirm")!="RESTART"){web_.send(400,"text/plain","Restart confirmation required");return;}
  if(ac_->session.pending||ac_->extended.pending){web_.send(409,"text/plain","AC command pending; try again shortly");return;}
  restart_=true;restart_at_=millis()+2000;
  web_.sendHeader("Cache-Control","no-store");
  web_.send(202,"application/json","{\"restarting\":true}");
 });
}
}
