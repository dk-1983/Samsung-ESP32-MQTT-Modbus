#pragma once
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/mqtt/mqtt_client.h"
#include "esphome/components/samsung_uart/samsung_uart.h"
#include "settings_model.h"
#include "credentials_model.h"
#include <WebServer.h>
#include "esphome/components/esphome/ota/ota_esphome.h"
namespace esphome::samsung_portal {
class Portal:public Component {
 public:
 void set_climate(samsung_uart::SamsungClimate *v){ac_=v;}
 void set_ota(ESPHomeOTAComponent *v){ota_=v;}
 void set_password(const std::string &v){password_=v.c_str();}
 void set_initial_ota(const std::string &v){initial_ota_=v.c_str();}
 void set_initial_setup(const std::string &v){initial_setup_=v.c_str();}
 void set_public_release(bool v){public_release_=v;}
 void ota_manual(bool value);
 float get_setup_priority()const override{return 150;}
 void setup()override;void loop()override;
 protected:
 samsung_uart::SamsungClimate *ac_=nullptr;ESPHomeOTAComponent *ota_=nullptr;
 samsung_management::Config config_{};ESPPreferenceObject pref_;
 WebServer web_{80};String password_,token_,initial_ota_,initial_setup_;
 samsung_credentials::Keys credentials_{};bool credentials_ok_=false;void credentials_web_();
 bool public_release_=false;void credentials_setup_();
 bool storage_ok_=true,mqtt_start_=false,web_started_=false,restart_=false;uint32_t restart_at_=0;
 bool test_auth_();bool post_auth_();void send_page_(const char *page);
 bool save_(const samsung_management::Config &c);void apply_mqtt_();
 uint32_t boot_id_=0;void system_web_();String system_json_();
 void settings_web_();String config_json_(bool mqtt);
 void updates_setup_();void updates_web_();void updates_loop_();bool updates_busy_();
};
}
