#include "samsung_mqtt.h"
#ifdef USE_MQTT
namespace esphome::samsung_uart {
void SamsungMqttClimate::setup(){
 // The base setup installs an unconditional publisher, so register commands here.
 disable_availability(); // Discovery below combines broker LWT and AC freshness.
 subscribe(get_mode_command_topic(),[this](const std::string &,const std::string &p){auto c=ac_->make_call();c.set_mode(p);c.perform();});
 subscribe(get_target_temperature_command_topic(),[this](const std::string &,const std::string &p){
  auto v=parse_number<float>(p);if(!v.has_value())return;auto c=ac_->make_call();c.set_target_temperature(*v);c.perform();
 });
 subscribe(get_fan_mode_command_topic(),[this](const std::string &,const std::string &p){auto c=ac_->make_call();c.set_fan_mode(p);c.perform();});
 subscribe(get_swing_mode_command_topic(),[this](const std::string &,const std::string &p){auto c=ac_->make_call();c.set_swing_mode(p);c.perform();});
 subscribe(get_preset_command_topic(),[this](const std::string &,const std::string &p){auto c=ac_->make_call();c.set_preset(p);c.perform();});
 ac_->add_on_state_callback([this](climate::Climate &){publish_feedback_();});
}
void SamsungMqttClimate::send_discovery(JsonObject root,mqtt::SendDiscoveryConfig &config){
 MQTTClimateComponent::send_discovery(root,config);
 auto list=root["availability"].to<JsonArray>();
 const auto &broker=mqtt::global_mqtt_client->get_availability();
 if(!broker.topic.empty()){
  auto item=list.add<JsonObject>();item["topic"]=broker.topic;
  item["payload_available"]=broker.payload_available;item["payload_not_available"]=broker.payload_not_available;
 }
 auto ac=list.add<JsonObject>();ac["topic"]=availability_topic_();
 root["availability_mode"]="all";
}
bool SamsungMqttClimate::publish_feedback_(){
 if(!mqtt::global_mqtt_client->is_connected())return false;
 const auto state=samsung_proto::mqtt_state(ac_->session.state,millis());
 // Go offline before clearing stale values; go online only after all state sends succeed.
 if((!state.available||available_<0)&&available_!=0){
  if(!publish(availability_topic_(),"offline"))return false;available_=0;
 }
 const std::array<std::string,6> topics={get_mode_state_topic(),get_target_temperature_state_topic(),
  get_current_temperature_state_topic(),get_fan_mode_state_topic(),get_swing_mode_state_topic(),get_preset_state_topic()};
 bool ok=true;
 // Preserve periodic valid telemetry for consumers that monitor publication age.
 // Unknown values need one reset, not a new message on every five-second tick.
 for(size_t i=0;i<topics.size();++i)if(cache_[i]!=state.fields[i]||state.fields[i]!="None"){
  if(publish(topics[i],state.fields[i]))cache_[i]=state.fields[i];else ok=false;
 }
 if(ok&&state.available&&available_!=1){
  if(publish(availability_topic_(),"online"))available_=1;else ok=false;
 }
 return ok;
}
}
#endif
