// SPDX-License-Identifier: GPL-3.0-or-later
#include "samsung_uart.h"
#include "esphome/core/log.h"
#include "esphome/components/wifi/wifi_component.h"
#include <cmath>
#include "esphome/components/uart/uart_component_esp_idf.h"
#include "driver/gpio.h"
namespace esphome { namespace samsung_uart {
using namespace samsung_proto;
using namespace climate;
static const char *const TAG="samsung_uart";
static std::string hex(const uint8_t *p,size_t n){
  static const char *digits="0123456789ABCDEF";std::string s;s.reserve(n*3);
  for(size_t i=0;i<n;++i){if(i)s+=' ';s+=digits[p[i]>>4];s+=digits[p[i]&15];}return s;
}
void SamsungClimate::setup(){
  set_supported_custom_presets({"quiet","legacy_smart","legacy_soft_cool","legacy_wind_1","legacy_wind_2","legacy_wind_3"});
  set_supported_custom_fan_modes({"turbo"});
  if(factory_){
    gpio_config_t cfg{};cfg.pin_bit_mask=(1ULL<<18)|(1ULL<<15);cfg.mode=GPIO_MODE_INPUT;
    cfg.pull_up_en=GPIO_PULLUP_DISABLE;cfg.pull_down_en=GPIO_PULLDOWN_DISABLE;
    if(gpio_config(&cfg)!=ESP_OK){mark_failed();return;}
  }
  enable_tx(true);current_temperature=NAN;target_temperature=NAN;
  ESP_LOGW(TAG,"Wi-Fi UART D0 profile: core controls tested on AR24BSFCMWKNER; extended features experimental. UART enabled at boot.");
}
void SamsungClimate::dump_config(){ESP_LOGCONFIG(TAG,"Samsung Wi-Fi UART prototype; Modbus unit %u, TCP 502, RTU 9600 8E1",unit_);}
ClimateTraits SamsungClimate::traits(){
  ClimateTraits t;t.set_feature_flags(CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  t.set_supported_modes({CLIMATE_MODE_OFF,CLIMATE_MODE_COOL,CLIMATE_MODE_HEAT,CLIMATE_MODE_DRY,CLIMATE_MODE_FAN_ONLY,CLIMATE_MODE_HEAT_COOL});
  t.set_supported_fan_modes({CLIMATE_FAN_LOW,CLIMATE_FAN_MEDIUM,CLIMATE_FAN_HIGH,CLIMATE_FAN_AUTO});
  t.set_supported_swing_modes({CLIMATE_SWING_OFF,CLIMATE_SWING_VERTICAL,CLIMATE_SWING_HORIZONTAL,CLIMATE_SWING_BOTH});
  t.set_supported_presets({CLIMATE_PRESET_NONE,CLIMATE_PRESET_BOOST,CLIMATE_PRESET_SLEEP});
  t.set_visual_min_temperature(16);t.set_visual_max_temperature(30);t.set_visual_temperature_step(1);return t;
}
void SamsungClimate::enable_tx(bool value){
  bridge_.enable(value,millis());link_.reset();session.enable(value);if(!value)extended.cancel();acks_.clear();reads_.clear();last_poll_=millis();
  ESP_LOGW(TAG,"UART transmission %s (not persisted)",value?"ENABLED":"DISABLED: MONITOR ONLY");
}
void SamsungClimate::bridge_emit_(unsigned destination,const uint8_t *p,size_t n,bool own){
  if(!session.enabled)return;
  (destination?factory_:parent_)->write_array(p,n);
  if(own){last_tx_ms_=millis();++tx_frames_;last_tx_=hex(p,n);ESP_LOGD(TAG,"TX own %s",last_tx_.c_str());}
}
bool SamsungClimate::transmission_ready_(){
  if(!session.enabled)return false;
  if(!factory_)return acks_.empty()&&!available()&&!parser_.used&&uint32_t(millis()-last_tx_ms_)>=300&&uint32_t(millis()-last_rx_ms_)>=30;
  if(!bridge_.ready(millis())||available()||factory_->available())return false;
  for(auto *b:{parent_,factory_}){
    auto *u=static_cast<uart::IDFUARTComponent *>(b);
    if(u->is_failed()||uart_wait_tx_done(static_cast<uart_port_t>(u->get_hw_serial_number()),0)!=ESP_OK)return false;
  }
  return true;
}
std::string SamsungClimate::bridge_diagnostics()const{
  if(!factory_)return "direct";
  char b[420];snprintf(b,sizeof(b),"inline RX18/TX17 RX15/TX16 | frames=%lu/%lu forwarded=%lu/%lu stock=%lu pending=%u held=%u errors=%lu expired=%lu own=%lu replies=%lu timeouts=%lu overflow=%lu",
    (unsigned long)bridge_.frames[0],(unsigned long)bridge_.frames[1],(unsigned long)bridge_.forwarded[0],(unsigned long)bridge_.forwarded[1],
    (unsigned long)bridge_.completed,unsigned(bridge_.pending_count()),unsigned(bridge_.held_count),(unsigned long)bridge_.errors,(unsigned long)bridge_.expired,
    (unsigned long)bridge_.own_sent,(unsigned long)bridge_.own_replies,(unsigned long)bridge_.own_timeouts,(unsigned long)bridge_.overflows);
  std::string out=b;
  for(auto *bus:{parent_,factory_,rs485_}){
    auto *u=static_cast<uart::IDFUARTComponent *>(bus);auto port=static_cast<uart_port_t>(u->get_hw_serial_number());uint32_t baud=0;
    int error=uart_get_baudrate(port,&baud);char v[80];snprintf(v,sizeof(v)," | UART%d baud=%lu error=%d failed=%d",int(port),(unsigned long)baud,error,int(u->is_failed()));out+=v;
  }
  return out;
}
bool SamsungClimate::send_(uint16_t type,const Bytes &payload,uint8_t counter){
  if(factory_){
    if(!transmission_ready_()||!bridge_.choose_counter(millis(),counter_))return false;
    return bridge_.send(type,payload,counter_,millis(),[this](unsigned d,const uint8_t *p,size_t n,bool own){bridge_emit_(d,p,n,own);});
  }
  if(!session.enabled || available() || parser_.used || uint32_t(millis()-last_tx_ms_)<300 || uint32_t(millis()-last_rx_ms_)<30)return false;
  Bytes b=frame(type,counter,payload);if(b.empty())return false;
  write_array(b.data(),b.size());last_tx_ms_=millis();++tx_frames_;last_tx_=hex(b.data(),b.size());
  ESP_LOGD(TAG,"TX %s",last_tx_.c_str());return true;
}
void SamsungClimate::query(){
  if(send_(0x1202,query_payload(),counter_)){++counter_;last_poll_=millis();}
}
void SamsungClimate::initialize_link(){
  if(factory_){ESP_LOGW(TAG,"Initialization belongs to the original Wi-Fi module in bridge mode");return;}
  // Deliberate separate operator action: known older Wi-Fi module initialization only.
  if(session.pending||extended.pending){ESP_LOGW(TAG,"Initialization rejected: command pending");return;}
  if(send_(0x1204,{0x01,1,0x0f,0x74,1,0xf0},counter_))++counter_;
  else ESP_LOGW(TAG,"Initialization not sent: enable TX and wait for idle UART");
}
bool SamsungClimate::submit(const Command &c){
  now=millis();
  if(extended.pending||!control_ready())return false;
  // Reserve session only when UART can transmit immediately; no stale command queue.
  if(!transmission_ready_())return false;
  if(!session.accept(c,now))return false;
  if(!send_(0x1204,command_payload(c),counter_)){session.pending=false;session.result=4;return false;}
  ++counter_;last_poll_=now-4500;return true;
}
bool SamsungClimate::submit_extra(size_t index,uint16_t value){
  now=millis();
  if(!session.enabled||!control_ready()||session.pending||extended.pending||!safely_fresh()||!extra_allowed(index,value))return false;
  if(!transmission_ready_())return false;
  if(factory_&&!bridge_.choose_counter(now,counter_))return false;
  if(!extended.accept(index,value,counter_,now))return false;
  auto &d=EXTRA[index];
  if(!send_(uint16_t(d.group)<<8|4,{d.id,1,uint8_t(value)},counter_)){extended.cancel();return false;}
  ++counter_;last_extra_poll_=now-500;return true;
}
void SamsungClimate::query_extended(){
  if(!session.enabled){ESP_LOGW(TAG,"Extended read rejected: monitor only");return;}
  reads_.clear();for(size_t i=0;i<EXTRA_COUNT;++i)reads_.push_back(i);
}
int SamsungClimate::extra_value(size_t index)const{
  if(index>=EXTRA_COUNT)return -1;const auto &v=extended.values[index];
  return v.fresh(millis())&&v.size==1?v.bytes[0]:-1;
}
std::string SamsungClimate::extra_state(size_t index)const{
  if(index>=EXTRA_COUNT)return "absent";const auto &v=extended.values[index];
  if(!v.known)return "unknown";
  return (v.fresh(millis())?std::string("raw "):std::string("stale raw "))+hex(v.bytes.data(),v.size);
}
std::string SamsungClimate::extra_status()const{
  static const char *names[]={"idle","pending","confirmed","timeout_unconfirmed","cancelled","acknowledged_unverified"};
  std::string s=names[extended.result];
  if(extended.index<EXTRA_COUNT){s+=" | ";s+=EXTRA[extended.index].name;}
  return s;
}
void SamsungClimate::control(const ClimateCall &call){
  Command c;auto set=[&](Field f,int v){c.mask|=1<<f;c.value[f]=v;};
  if(call.get_mode()){
    auto m=*call.get_mode();set(POWER,m!=CLIMATE_MODE_OFF);
    if(m!=CLIMATE_MODE_OFF){int v=m==CLIMATE_MODE_COOL?1:m==CLIMATE_MODE_HEAT?2:m==CLIMATE_MODE_DRY?3:m==CLIMATE_MODE_FAN_ONLY?4:m==CLIMATE_MODE_HEAT_COOL?5:0;
      if(!v)return;set(MODE,v);}
  }
  if(call.get_target_temperature()){
    float v=*call.get_target_temperature();if(!std::isfinite(v)||v!=std::floor(v)||v<16||v>30)return;set(TARGET,int(v));
  }
  if(call.get_fan_mode()){
    auto v=*call.get_fan_mode();int f=v==CLIMATE_FAN_LOW?1:v==CLIMATE_FAN_MEDIUM?2:v==CLIMATE_FAN_HIGH?3:v==CLIMATE_FAN_AUTO?4:0;if(!f)return;set(FAN,f);
  }
  if(call.has_custom_fan_mode()){
    if(call.get_custom_fan_mode()!="turbo")return;set(FAN,5);
  }
  if(call.get_swing_mode()){
    auto v=*call.get_swing_mode();set(SWING,v==CLIMATE_SWING_OFF?0:v==CLIMATE_SWING_VERTICAL?1:v==CLIMATE_SWING_HORIZONTAL?2:3);
  }
  if(call.get_preset()){
    auto v=*call.get_preset();int p=v==CLIMATE_PRESET_NONE?0:v==CLIMATE_PRESET_BOOST?1:v==CLIMATE_PRESET_SLEEP?2:-1;
    if(p<0)return;set(PRESET,p);
  }
  if(call.has_custom_preset()){
    const char *names[]={"quiet","legacy_smart","legacy_soft_cool","legacy_wind_1","legacy_wind_2","legacy_wind_3"};
    int value=-1;for(int i=0;i<6;++i)if(call.get_custom_preset()==names[i])value=i+3;
    if(value<0)return;set(PRESET,value);
  }
  if(!submit(c))ESP_LOGW(TAG,"Command rejected: monitor mode, stale/unknown field, pending command or UART busy");
  // Never publish requested values optimistically; only received feedback is state.
}
void SamsungClimate::received_(const uint8_t *p,size_t n,bool own){
  last_rx_=hex(p,n);ESP_LOGD(TAG,"RX valid %s",last_rx_.c_str());
  if(p[10]!=0xfe){++other_frames_;return;} // Observed FC service frames: capture only, no guessed ACK.
  now=millis();link_.receive(p,n,now);
  if(factory_&&!own){
    session.state.update(p,n,now);
    bool pending=extended.pending;extended.pending=false;extended.receive(p,n,now);extended.pending=pending;
  }else {session.receive(p,n,now);extended.receive(p,n,now);}
  if(p[12]==5){last_write_reply_=last_rx_;if(factory_&&own){last_poll_=now-5000;last_extra_poll_=now-800;}}
  if(p[11]==0x12 && (p[12]==3||p[12]==6)){
    ++status_count;seen_status=true;last_status=now;publish_feedback_();
  }
  // ACK unsolicited status only. Read responses and write ACKs must never be ACKed.
  if(!factory_ && session.enabled && p[11]>=0x12 && p[11]<=0x14 && p[12]==6 && acks_.size()<4)
    acks_.push_back({Bytes(p+14,p+n-2),p[9],uint16_t(uint16_t(p[11])<<8|7)});
}
void SamsungClimate::publish_feedback_(){
  const auto &s=session.state;uint32_t t=millis();
  target_temperature=s.fresh(TARGET,t)?s.value[TARGET]:NAN;
  current_temperature=s.fresh(ROOM,t)?s.value[ROOM]:NAN;
  if(s.fresh(POWER,t)&&s.fresh(MODE,t)){
    static const ClimateMode modes[]={CLIMATE_MODE_OFF,CLIMATE_MODE_COOL,CLIMATE_MODE_HEAT,CLIMATE_MODE_DRY,CLIMATE_MODE_FAN_ONLY,CLIMATE_MODE_HEAT_COOL};
    mode=s.value[POWER]?modes[s.value[MODE]]:CLIMATE_MODE_OFF;
  }
  if(s.fresh(FAN,t)){
    static const ClimateFanMode fans[]={CLIMATE_FAN_AUTO,CLIMATE_FAN_LOW,CLIMATE_FAN_MEDIUM,CLIMATE_FAN_HIGH,CLIMATE_FAN_AUTO};
    if(s.value[FAN]==5)set_custom_fan_mode_("turbo");else set_fan_mode_(fans[s.value[FAN]]);
  }else {fan_mode.reset();clear_custom_fan_mode_();}
  if(s.fresh(SWING,t)){
    static const ClimateSwingMode swings[]={CLIMATE_SWING_OFF,CLIMATE_SWING_VERTICAL,CLIMATE_SWING_HORIZONTAL,CLIMATE_SWING_BOTH};swing_mode=swings[s.value[SWING]];
  }
  if(s.fresh(PRESET,t)){
    static const ClimatePreset presets[]={CLIMATE_PRESET_NONE,CLIMATE_PRESET_BOOST,CLIMATE_PRESET_SLEEP};
    static const char *custom[]={"quiet","legacy_smart","legacy_soft_cool","legacy_wind_1","legacy_wind_2","legacy_wind_3"};
    if(s.value[PRESET]>=3)set_custom_preset_(custom[s.value[PRESET]-3]);else set_preset_(presets[s.value[PRESET]]);
  }else {preset.reset();clear_custom_preset_();}
  publish_state();last_publish_=t;
}
std::string SamsungClimate::command_status()const{
  const char *names[]={"idle","pending","confirmed","timeout_unconfirmed","cancelled"};return names[session.result];
}
std::string SamsungClimate::diagnostics()const{
  char b[240];snprintf(b,sizeof(b),"%s | RX bytes=%lu valid=%lu invalid=%lu TX=%lu | feedback=%s | cmd=%s | service=%lu",
    session.enabled?"ACTIVE D0 profile":"MONITOR ONLY",(unsigned long)rx_bytes_,(unsigned long)(factory_?bridge_.frames[0]:parser_.frames),
    (unsigned long)(factory_?bridge_.errors:parser_.bad),(unsigned long)tx_frames_,feedback_fresh()?"fresh":"unknown/stale",command_status().c_str(),(unsigned long)other_frames_);return b;
}
void SamsungClimate::loop(){
  now=millis();session.tick(now);extended.tick(now);
  if(factory_){
    auto emit=[this](unsigned d,const uint8_t *p,size_t n,bool own){bridge_emit_(d,p,n,own);};
    for(unsigned c=0;c<2;++c){auto *bus=c?factory_:parent_;
      for(unsigned budget=0;budget<48&&bus->available();++budget){uint8_t v;if(!bus->read_byte(&v))break;
        if(c==0){++rx_bytes_;last_rx_ms_=millis();}
        bridge_.feed(c,v,millis(),emit,[this](const uint8_t *p,size_t n,bool own){received_(p,n,own);});
      }
    }
    bridge_.tick(millis(),emit);
  }else {
  if(parser_.used && uint32_t(now-last_rx_ms_)>200){parser_.clear();++parser_.bad;}
  if(raw_used_ && uint32_t(now-last_rx_ms_)>200){last_rx_=hex(raw_,raw_used_);ESP_LOGD(TAG,"RX bytes %s",last_rx_.c_str());raw_used_=0;}
  for(unsigned budget=0;budget<512 && available();++budget){
    uint8_t v;if(!read_byte(&v))break;last_rx_ms_=millis();++rx_bytes_;raw_[raw_used_++]=v;
    if(raw_used_==sizeof(raw_)){last_rx_=hex(raw_,raw_used_);ESP_LOGD(TAG,"RX bytes %s",last_rx_.c_str());raw_used_=0;}
    parser_.push(v,[this](const uint8_t *p,size_t n){received_(p,n);});
  }
  }
  if(session.enabled){
    if(!acks_.empty()){
      if(send_(acks_.front().type,acks_.front().payload,acks_.front().counter))acks_.pop_front();
    }else if(!factory_&&!session.pending&&!extended.pending&&link_.needs_enable(now)){
      // Restore only control permission; leave beep and HVAC settings untouched.
      if(send_(0x1204,{0x01,1,0x0f},counter_)){
        ++counter_;link_.sent(now);last_poll_=now-4500;
        ESP_LOGW(TAG,"Restoring UART control permission; waiting for readback");
      }
    }else if(uint32_t(now-last_poll_)>=5000)query();
    else if(!session.pending && uint32_t(now-last_extra_poll_)>=800 && (extended.pending||!reads_.empty()||extended_poll_)){
      uint8_t i=extended.pending?extended.index:!reads_.empty()?reads_.front():poll_index_;
      auto &d=EXTRA[i];
      if(send_(uint16_t(d.group)<<8|2,{d.id,0},counter_)){
        ++counter_;last_extra_poll_=now;
        if(!extended.pending){if(!reads_.empty())reads_.pop_front();else poll_index_=(poll_index_+1)%EXTRA_COUNT;}
      }
    }
  }
  if(uint32_t(now-last_publish_)>=5000)publish_feedback_();
  transports_();
}
void SamsungClimate::configure_modbus(bool rtu,bool tcp,uint8_t unit,uint32_t baud){
  for(auto &c:clients_){c.socket.stop();c.frame.clear();}
  if(listening_){server_.end();listening_=false;}
  rtu_enabled=rtu;tcp_enabled=tcp;unit_=unit;rtu_used_=0;rtu_overflow_=false;
  rs485_->set_baud_rate(baud);rs485_->load_settings(false);
  rtu_gap_us_=baud>19200?1750:(38500000UL+baud-1)/baud;
}
void SamsungClimate::transports_(){
  // ESPHome owns esp_netif; Arduino's separate STA status is not initialized by it.
  auto *wifi=wifi::global_wifi_component;
  bool connected=wifi!=nullptr && wifi->is_connected();
  if(tcp_enabled&&connected&&!listening_){server_.begin();listening_=true;}
  if((!tcp_enabled||!connected)&&listening_){for(auto &c:clients_)c.socket.stop();server_.end();listening_=false;}
  if(listening_){
    WiFiClient incoming=server_.accept();
    if(incoming){bool assigned=false;
      if(std::string(incoming.localIP().toString().c_str())==wifi->get_ip_addresses()[0].str())for(auto &c:clients_)if(!c.socket.connected()){
        c.socket.stop();c.socket=incoming;c.socket.setTimeout(100);c.frame.clear();c.last=now;assigned=true;break;
      }if(!assigned)incoming.stop();
    }
    for(auto &c:clients_){
      if(!c.socket.connected())continue;
      if(uint32_t(now-c.last)>(c.frame.used?2000:30000)){c.socket.stop();continue;}
      for(unsigned budget=0;budget<260&&c.socket.available();++budget){
        int b=c.socket.read();if(b<0)break;c.last=now;int result=c.frame.push(uint8_t(b));
        if(result<0){c.socket.stop();break;}
        if(result==1){uint8_t out[260];size_t n=samsung_modbus::tcp(*this,c.frame.bytes,c.frame.used,unit_,out);c.frame.clear();
          if(!n||c.socket.write(out,n)!=n){c.socket.stop();break;}}
      }
    }
  }
  if(!rtu_enabled){for(unsigned budget=0;budget<512&&rs485_->available();++budget){uint8_t v;rs485_->read_byte(&v);}rtu_used_=0;rtu_overflow_=false;return;}
  auto dispatch=[this](){uint8_t out[256];size_t n=samsung_modbus::rtu(*this,rtu_,rtu_used_,unit_,out);
    if(n){rs485_->write_array(out,n);rs485_->flush();}rtu_used_=0;};
  // Fixed 9600 8E1 profile (3.5 characters = 4011 us). Hardware RX timeout configured to 2 characters.
  if(uint32_t(micros()-rtu_last_)>=rtu_gap_us_ && !rs485_->available()){
    if(rtu_used_&&!rtu_overflow_)dispatch();rtu_used_=0;rtu_overflow_=false;
  }
  for(unsigned budget=0;budget<512&&rs485_->available();++budget){
    uint8_t v;if(!rs485_->read_byte(&v))break;rtu_last_=micros();
    if(rtu_overflow_)continue;
    if(rtu_used_==sizeof(rtu_)){rtu_overflow_=true;rtu_used_=0;continue;}rtu_[rtu_used_++]=v;
    if(rtu_used_>=2){uint8_t fc=rtu_[1];size_t wanted=(fc==1||fc==3||fc==4||fc==5||fc==6)?8:
      ((fc==15||fc==16)&&rtu_used_>=7)?size_t(9+rtu_[6]):0;
      if(wanted>256){rtu_overflow_=true;rtu_used_=0;continue;}
      if(wanted&&rtu_used_==wanted)dispatch();
    }
  }
}
}}
