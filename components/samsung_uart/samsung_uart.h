// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"
#include "bridge.h"
#include <WiFi.h>
#include <deque>
namespace esphome { namespace samsung_uart {
class SamsungClimate : public Component, public climate::Climate, public uart::UARTDevice, public samsung_proto::Bridge {
 public:
  void set_rs485(uart::UARTComponent *v){rs485_=v;}
  void set_unit(uint8_t v){unit_=v;}
  void configure_modbus(bool rtu,bool tcp,uint8_t unit,uint32_t baud);
  uint32_t rtu_gap_us_=4100;
  void setup()override;
  void loop()override;
  void dump_config()override;
  bool submit(const samsung_proto::Command &c)override;
  bool submit_extra(size_t index,uint16_t value)override;
  void query_extended();
  void set_extended_poll(bool value){extended_poll_=value;}
  bool extended_poll()const{return extended_poll_;}
  std::string extra_state(size_t index)const;
  int extra_value(size_t index)const;
  std::string extra_status()const;
  std::string last_write_reply()const{return last_write_reply_;}
  void enable_tx(bool value);
  bool tx_enabled()const{return session.enabled;}
  void query();
  void initialize_link();
  std::string diagnostics() const;
  std::string last_rx()const{return last_rx_;}
  std::string last_tx()const{return last_tx_;}
  std::string command_status()const;
  bool feedback_fresh()const{return session.state.fresh_mask(samsung_proto::CORE,millis());}
 protected:
  climate::ClimateTraits traits()override;
  void control(const climate::ClimateCall &call)override;
  void received_(const uint8_t *p,size_t n);
  void publish_feedback_();
  void transports_();
  bool send_(uint16_t type,const samsung_proto::Bytes &payload,uint8_t counter);
  uart::UARTComponent *rs485_=nullptr;
  samsung_proto::Parser parser_;
  uint32_t rx_bytes_=0,tx_frames_=0,last_rx_ms_=0,last_tx_ms_=0,last_poll_=0,last_publish_=0;
  uint8_t counter_=0,unit_=1;
  struct Ack {samsung_proto::Bytes payload;uint8_t counter;uint16_t type;};
  std::deque<Ack> acks_;
  std::deque<uint8_t> reads_;
  bool extended_poll_=false;
  uint8_t poll_index_=0;
  uint32_t last_extra_poll_=0,other_frames_=0;
  std::string last_write_reply_;
  std::string last_rx_,last_tx_;
  uint8_t raw_[48]{};size_t raw_used_=0;
  WiFiServer server_{502};bool listening_=false;
  struct Client {WiFiClient socket;samsung_modbus::TcpFrame frame;uint32_t last=0;} clients_[2];
  uint8_t rtu_[256]{};size_t rtu_used_=0;uint32_t rtu_last_=0;bool rtu_overflow_=false;
};
}}
