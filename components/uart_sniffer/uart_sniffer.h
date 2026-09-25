#pragma once
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include <WebServer.h>
namespace esphome::uart_sniffer {
class Sniffer : public Component {
 public:
  void set_rx_a(uart::UARTComponent *v){buses_[0]=v;}
  void set_rx_b(uart::UARTComponent *v){buses_[1]=v;}
  void set_password(const std::string &v){password_=v.c_str();}
  void setup()override;
  void loop()override;
 protected:
  struct Chunk{uint32_t seq=0,at=0,end=0;uint8_t channel=0,size=0,data[48]{};};
  Chunk ring_[128]{},pending_[2]{};
  uart::UARTComponent *buses_[2]{};
  uint32_t seq_=0,bytes_[2]{},boot_=0;
  WebServer web_{80};String password_;bool web_started_=false;
  void flush_(unsigned channel);
  bool auth_(){if(web_.authenticate("admin",password_.c_str()))return true;web_.requestAuthentication();return false;}
  String capture_();
};
}
