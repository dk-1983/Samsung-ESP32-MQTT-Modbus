#pragma once
#include "samsung_uart.h"
#include "mqtt_state.h"
#include "esphome/components/mqtt/mqtt_climate.h"
#ifdef USE_MQTT
namespace esphome::samsung_uart {
class SamsungMqttClimate : public mqtt::MQTTClimateComponent {
 public:
 explicit SamsungMqttClimate(SamsungClimate *ac):MQTTClimateComponent(ac),ac_(ac){}
 void setup() override;
 void send_discovery(JsonObject root,mqtt::SendDiscoveryConfig &config) override;
 bool send_initial_state() override {cache_.fill("");available_=-1;return publish_feedback_();}
 protected:
 SamsungClimate *ac_;
 std::array<std::string,6> cache_{};
 int available_=-1;
 std::string availability_topic_(){return get_mode_state_topic()+"/availability";}
 bool publish_feedback_();
};
}
#endif
