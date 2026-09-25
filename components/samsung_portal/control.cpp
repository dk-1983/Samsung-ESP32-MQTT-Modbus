// SPDX-License-Identifier: GPL-3.0-or-later
#include "samsung_portal.h"
#include "ControlPage.h"
#include <cJSON.h>
namespace esphome::samsung_portal {
String Portal::control_json_(){
 using namespace samsung_proto;auto &s=ac_->session.state;uint32_t now=millis();
 cJSON *j=cJSON_CreateObject();if(!j)return "{}";
 cJSON_AddBoolToObject(j,"ac_fresh",ac_->feedback_fresh());
 cJSON_AddBoolToObject(j,"uart",ac_->tx_enabled());
 cJSON_AddBoolToObject(j,"control_ready",ac_->control_ready());
 cJSON_AddBoolToObject(j,"mqtt_enabled",config_.mqtt);
 cJSON_AddBoolToObject(j,"mqtt_connected",mqtt::global_mqtt_client->is_connected());
 bool busy=ac_->session.pending||ac_->extended.pending||updates_busy_()||restart_;
 cJSON_AddBoolToObject(j,"busy",busy);
 cJSON_AddBoolToObject(j,"can_command",!busy&&ac_->tx_enabled()&&ac_->control_ready()&&s.fresh_mask(CORE,now,10000));
 cJSON_AddStringToObject(j,"command_result",ac_->command_status().c_str());
 cJSON_AddStringToObject(j,"extended_result",ac_->extra_status().c_str());
 const char *keys[]={"power","target","mode","fan","swing","preset","room"};
 for(unsigned i=0;i<FIELD_COUNT;++i){if(s.fresh(Field(i),now))cJSON_AddNumberToObject(j,keys[i],s.value[i]);else cJSON_AddNullToObject(j,keys[i]);}
 char *raw=cJSON_PrintUnformatted(j);String result=raw?raw:"{}";if(raw)cJSON_free(raw);cJSON_Delete(j);return result;
}
void Portal::control_web_(){
 web_.on("/",HTTP_GET,[this](){if(test_auth_())send_page_(OVERVIEW_PAGE);});
 web_.on("/control",HTTP_GET,[this](){if(test_auth_())send_page_(CONTROL_PAGE);});
 web_.on("/control/status",HTTP_GET,[this](){if(!test_auth_())return;web_.sendHeader("Cache-Control","no-store");web_.send(200,"application/json",control_json_());});
 web_.on("/control/set",HTTP_POST,[this](){
  if(!post_auth_())return;
  if(web_.args()!=3||!web_.hasArg("field")||!web_.hasArg("value")){web_.send(400,"text/plain","Expected field and value");return;}
  String field=web_.arg("field");uint32_t v;
  if(!samsung_management::numeric(web_.arg("value").c_str(),0,255,v)){web_.send(400,"text/plain","Invalid value");return;}
  using namespace samsung_proto;bool accepted=false;
  if(field=="power"){
   if(v>1){web_.send(400,"text/plain","Power must be 0 or 1");return;}
   accepted=ac_->set_power(v!=0);
  }else{
   Field f=field=="target"?TARGET:field=="mode"?MODE:field=="fan"?FAN:field=="swing"?SWING:field=="preset"?PRESET:ROOM;
   if(f==ROOM||!allowed(f,v)){web_.send(400,"text/plain","Invalid field or value");return;}
   Command c;c.mask=1<<f;c.value[f]=v;accepted=ac_->submit(c);
  }
  web_.sendHeader("Cache-Control","no-store");
  if(!accepted){web_.send(409,"text/plain","UART disabled, busy or feedback stale");return;}
  web_.send(202,"application/json","{\"accepted\":true,\"confirmed\":false}");
 });
}
}
