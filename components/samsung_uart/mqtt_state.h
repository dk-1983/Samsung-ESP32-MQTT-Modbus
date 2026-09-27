#pragma once
#include "protocol.h"
#include <string>
namespace samsung_proto {
// HA MQTT climate uses the literal None to clear an unknown field.
struct MqttState {
 std::array<std::string,6> fields; // mode, target, room, fan, swing, preset
 bool available=false;
};
inline MqttState mqtt_state(const State &s,uint32_t now){
 MqttState out;out.fields.fill("None");
 auto valid=[&](Field f){return s.fresh(f,now)&&(f==ROOM||allowed(f,s.value[f]));};
 out.available=valid(POWER)&&valid(MODE)&&valid(TARGET)&&valid(FAN);
 if(valid(POWER)&&(!s.value[POWER]||valid(MODE))){
  static const char *modes[]={"off","cool","heat","dry","fan_only","heat_cool"};
  out.fields[0]=s.value[POWER]?modes[s.value[MODE]]:"off";
 }
 if(valid(TARGET))out.fields[1]=std::to_string(s.value[TARGET]);
 if(valid(ROOM))out.fields[2]=std::to_string(s.value[ROOM]);
 if(valid(FAN)){
  static const char *fans[]={"None","low","medium","high","auto","turbo"};
  out.fields[3]=fans[s.value[FAN]];
 }
 if(valid(SWING)){
  static const char *swings[]={"off","vertical","horizontal","both"};
  out.fields[4]=swings[s.value[SWING]];
 }
 if(valid(PRESET)){
  static const char *presets[]={"none","boost","sleep","quiet","legacy_smart","legacy_soft_cool","legacy_wind_1","legacy_wind_2","legacy_wind_3"};
  out.fields[5]=presets[s.value[PRESET]];
 }
 return out;
}
}
