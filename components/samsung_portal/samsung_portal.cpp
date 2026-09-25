// SPDX-License-Identifier: GPL-3.0-or-later
#include "samsung_portal.h"
#include "esphome/components/wifi/wifi_component.h"
#include "esphome/core/application.h"
#include <esp_random.h>
#include <cJSON.h>
#include <Preferences.h>
#include "esphome/components/web_server_base/web_server_base.h"
#include "UiShell.h"
#include "MqttPage.h"
#include "ModbusPage.h"
namespace esphome::samsung_portal {
static const char HOME[] PROGMEM=R"HTML(<!doctype html><html lang="ru"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>4vrs Samsung-ESP32</title>__STYLE__<body>__NAV__<main><h1>Samsung-ESP32</h1><p>Локальное управление кондиционером</p><div class="grid"><a class="card" href="/control">Веб-пульт</a><a class="card" href="/mqtt">Подключение MQTT</a><a class="card" href="/modbus">Modbus RTU / TCP</a><a class="card" href="/updates">Обновления GitHub</a><a class="card" href="/about">О системе и перезагрузка</a></div><p>MQTT и оба транспорта Modbus включаются независимо. Настройки сохраняются после отключения питания.</p></main></body></html>)HTML";
bool Portal::test_auth_(){if(web_.authenticate("admin",password_.c_str()))return true;web_.requestAuthentication();return false;}
bool Portal::post_auth_(){if(!test_auth_())return false;if(web_.arg("token")!=token_){web_.send(403,"text/plain","Invalid token");return false;}if(updates_busy_()||restart_){web_.send(409,"text/plain","Update or restart in progress");return false;}return true;}
void Portal::send_page_(const char *page){
 String body=FPSTR(page);body.replace("__TOKEN__",token_);body.replace("__STYLE__",FPSTR(UI_STYLE));body.replace("__NAV__",FPSTR(UI_NAV));
 web_.sendHeader("Cache-Control","no-store");web_.send(200,"text/html; charset=utf-8",body);
}
bool Portal::save_(const samsung_management::Config &c){
 if(!samsung_management::valid(c)||!pref_.save(&c)||!global_preferences->sync())return false;config_=c;return true;
}
void Portal::apply_mqtt_(){
 auto *m=mqtt::global_mqtt_client;m->disable();
 m->set_broker_address(config_.host[0]?config_.host:"192.0.2.1");m->set_broker_port(config_.port);
 m->set_username(config_.username);m->set_password(config_.password);m->set_topic_prefix(config_.prefix,config_.prefix);
 m->set_birth_message({std::string(config_.prefix)+"/status","online",0,true});
 m->set_last_will({std::string(config_.prefix)+"/status","offline",0,true});
 m->set_shutdown_message({std::string(config_.prefix)+"/status","offline",0,true});
 if(config_.discovery)m->set_discovery_info("homeassistant",mqtt::MQTT_MAC_ADDRESS_UNIQUE_ID_GENERATOR,mqtt::MQTT_DEVICE_NAME_OBJECT_ID_GENERATOR,true,false,false);
 else m->disable_discovery();
 mqtt_start_=config_.mqtt;
}
String Portal::config_json_(bool is_mqtt){
 cJSON *j=cJSON_CreateObject();
 if(is_mqtt){
  cJSON_AddBoolToObject(j,"enabled",config_.mqtt);cJSON_AddBoolToObject(j,"discovery",config_.discovery);
  cJSON_AddBoolToObject(j,"connected",mqtt::global_mqtt_client->is_connected());
  cJSON_AddStringToObject(j,"host",config_.host);cJSON_AddNumberToObject(j,"port",config_.port);
  cJSON_AddStringToObject(j,"username",config_.username);cJSON_AddStringToObject(j,"prefix",config_.prefix);
  cJSON_AddBoolToObject(j,"password_set",config_.password[0]!=0);
 }else{
  cJSON_AddBoolToObject(j,"rtu",config_.rtu);cJSON_AddBoolToObject(j,"tcp",config_.tcp);
  cJSON_AddNumberToObject(j,"unit",config_.unit);cJSON_AddNumberToObject(j,"baud",config_.baud);
 }
 char *raw=cJSON_PrintUnformatted(j);String s=raw?raw:"{}";cJSON_free(raw);cJSON_Delete(j);return s;
}
void Portal::settings_web_(){
 web_.on("/",HTTP_GET,[this](){if(test_auth_())send_page_(HOME);});
 web_.on("/control",HTTP_GET,[this](){if(!test_auth_())return;auto *w=wifi::global_wifi_component;char ip[network::IP_ADDRESS_BUFFER_SIZE];(w->is_connected()?w->get_ip_addresses()[0]:w->wifi_soft_ap_ip()).str_to(ip);web_.sendHeader("Location",String("http://")+ip+":8080/");web_.send(302,"text/plain","");});
 web_.on("/wifi",HTTP_GET,[this](){web_.sendHeader("Location","http://192.168.4.1:8080/");web_.send(302,"text/plain","");});
 web_.on("/mqtt",HTTP_GET,[this](){if(test_auth_())send_page_(MQTT_PAGE);});
 web_.on("/modbus",HTTP_GET,[this](){if(test_auth_())send_page_(MODBUS_PAGE);});
 web_.on("/mqtt/config",HTTP_GET,[this](){if(!test_auth_())return;web_.sendHeader("Cache-Control","no-store");web_.send(200,"application/json",config_json_(true));});
 web_.on("/modbus/config",HTTP_GET,[this](){if(!test_auth_())return;web_.sendHeader("Cache-Control","no-store");web_.send(200,"application/json",config_json_(false));});
 web_.on("/modbus/config",HTTP_POST,[this](){
  if(!post_auth_())return;auto c=config_;uint32_t unit=0,baud=0;
  auto flag=[this](const char *k){return web_.arg(k)=="0"||web_.arg(k)=="1";};
  if(!flag("rtu")||!flag("tcp")||!samsung_management::numeric(web_.arg("unit").c_str(),1,247,unit)||!samsung_management::numeric(web_.arg("baud").c_str(),9600,115200,baud)||!samsung_management::baud(baud)){web_.send(400,"text/plain","Invalid Modbus settings");return;}
  c.rtu=web_.arg("rtu")=="1";c.tcp=web_.arg("tcp")=="1";c.unit=unit;c.baud=baud;
  if(!save_(c)){web_.send(503,"text/plain","Settings not saved");return;}
  ac_->configure_modbus(c.rtu,c.tcp,c.unit,c.baud);web_.send(200,"application/json","{}");
 });
 web_.on("/mqtt/config",HTTP_POST,[this](){
  if(!post_auth_())return;auto c=config_;uint32_t port=0;
  auto flag=[this](const char *k){return web_.arg(k)=="0"||web_.arg(k)=="1";};
  if(!flag("enabled")||!flag("discovery")||!flag("clear_password")||!samsung_management::numeric(web_.arg("port").c_str(),1,65535,port)||web_.arg("host").length()>127||web_.arg("username").length()>64||web_.arg("password").length()>128||web_.arg("prefix").length()>64){web_.send(400,"text/plain","Invalid MQTT settings");return;}
  c.port=port;c.mqtt=web_.arg("enabled")=="1";c.discovery=web_.arg("discovery")=="1";
  strlcpy(c.host,web_.arg("host").c_str(),sizeof(c.host));strlcpy(c.username,web_.arg("username").c_str(),sizeof(c.username));strlcpy(c.prefix,web_.arg("prefix").c_str(),sizeof(c.prefix));
  if(web_.arg("clear_password")=="1")c.password[0]=0;else if(web_.arg("password").length())strlcpy(c.password,web_.arg("password").c_str(),sizeof(c.password));
  if(!samsung_management::valid(c)){web_.send(400,"text/plain","Invalid host or topic prefix");return;}
  if(!save_(c)){web_.send(503,"text/plain","Settings not saved");return;}
  // ESPHome's ESP32 MQTT backend initializes its IDF client only once.
  // disable()/enable() does not recreate it with changed credentials/server.
  // Persist first, then reboot after the HTTP response to apply all MQTT settings.
  mqtt::global_mqtt_client->disable();
  mqtt_start_=false;restart_=true;restart_at_=millis()+2000;
  web_.send(200,"application/json","{}");
 });
 web_.onNotFound([this](){if(test_auth_())web_.send(404,"text/plain","Not found");});
}
void Portal::credentials_setup_(){
 struct Keys{uint32_t magic=0x534b5901;char web[65]{},ota[65]{},setup[64]{};} keys;
 ::Preferences store;bool ok=store.begin("samsung-keys",false);
 if(ok&&store.isKey("keys"))ok=store.getBytesLength("keys")==sizeof(keys)&&store.getBytes("keys",&keys,sizeof(keys))==sizeof(keys)&&keys.magic==0x534b5901&&memchr(keys.web,0,sizeof(keys.web))&&memchr(keys.ota,0,sizeof(keys.ota))&&memchr(keys.setup,0,sizeof(keys.setup))&&strlen(keys.web)>=8&&strlen(keys.ota)>=16&&strlen(keys.setup)>=8;
 else if(ok&&!public_release_){
  strlcpy(keys.web,password_.c_str(),sizeof(keys.web));strlcpy(keys.ota,initial_ota_.c_str(),sizeof(keys.ota));strlcpy(keys.setup,initial_setup_.c_str(),sizeof(keys.setup));
  ok=store.putBytes("keys",&keys,sizeof(keys))==sizeof(keys);
 }else ok=false;
 if(!ok){
  storage_ok_=false;
  // A public OTA is for an already provisioned controller; never expose fallback credentials.
  char random[65];for(int i=0;i<8;++i)snprintf(random+8*i,9,"%08lx",(unsigned long)esp_random());
  password_=random;ota_->set_auth_password(random);
 }else{password_=keys.web;ota_->set_auth_password(keys.ota);auto *w=wifi::global_wifi_component;auto ap=w->get_ap();ap.set_password(keys.setup);w->set_ap(ap);}
 web_server_base::global_web_server_base->set_auth_password(password_.c_str());
}
void Portal::setup(){
 pref_=global_preferences->make_preference<samsung_management::Config>(0x534d4701);
 samsung_management::Config saved;if(pref_.load(&saved)&&samsung_management::valid(saved))config_=saved;
 else {storage_ok_=pref_.save(&config_)&&global_preferences->sync();}
 credentials_setup_();boot_id_=esp_random();
 char token[33];for(int i=0;i<4;++i)snprintf(token+8*i,9,"%08lx",(unsigned long)esp_random());token_=token;
 apply_mqtt_();ac_->configure_modbus(config_.rtu,config_.tcp,config_.unit,config_.baud);
 settings_web_();system_web_();updates_web_();updates_setup_();
}
void Portal::loop(){
 auto *w=wifi::global_wifi_component;bool connected=w&&w->is_connected();
 if((connected||(w&&w->is_ap_active()))&&!web_started_){web_.begin();web_started_=true;}
 if(web_started_)web_.handleClient();
 if(mqtt_start_&&!restart_){mqtt_start_=false;mqtt::global_mqtt_client->enable();}
 updates_loop_();
 if(restart_&&int32_t(millis()-restart_at_)>=0&&!updates_busy_())App.safe_reboot();
}
}
