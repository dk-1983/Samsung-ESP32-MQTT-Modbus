#pragma once
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include <WebServer.h>
#include <esp_attr.h>
namespace esphome::uart_sniffer {
class Sniffer : public Component {
 public:
  void set_rx_a(uart::UARTComponent *v){buses_[0]=v;}
  void set_rx_c(uart::UARTComponent *v){buses_[2]=v;}
  void set_rx_b(uart::UARTComponent *v){buses_[1]=v;}
  void set_password(const std::string &v){password_=v.c_str();}
  void set_rx_b_gpio(int v){rx_b_gpio_=v;}
  void set_factory_rx_gpio(int v){factory_rx_gpio_=v;}
  void set_factory_tx_gpio(int v){factory_tx_gpio_=v;}
  void set_bridge(bool v){bridge_=v;}
  void set_forwarding(bool v){forwarding_=v&&!monitor_only_;}
  void set_monitor_only(bool v){monitor_only_=v;if(v)forwarding_=false;}
  void setup()override;
  void loop()override;
 protected:
  struct Chunk{uint32_t seq=0,at=0,end=0;uint8_t channel=0,size=0,data[48]{};};
  Chunk ring_[128]{},pending_[3]{};
  uart::UARTComponent *buses_[3]{};
  uint32_t seq_=0,bytes_[3]{},boot_=0;
  bool bridge_=false,forwarding_=true;
  bool monitor_only_=false;
  int rx_b_gpio_=-1;
  int factory_rx_gpio_=15,factory_tx_gpio_=16;
  int loopback_result_[2]{-1,-1};
  volatile uint32_t rx_edges_[3]{};
  int edge_errors_[3]{-1,-1,-1};
  bool probe_sent_=false;
  uint32_t probe_at_=0;
  static void IRAM_ATTR on_edge_(void *arg);
  uint32_t forwarded_[3]{};
  WebServer web_{80};String password_;bool web_started_=false;
  unsigned bus_count_()const{return buses_[2]?3:2;}
  int rx_gpio_(unsigned c)const{return c==2?9:(c?(rx_b_gpio_>=0?rx_b_gpio_:(bridge_?factory_rx_gpio_:17)):18);}
  void flush_(unsigned channel);
  bool auth_(){if(web_.authenticate("admin",password_.c_str()))return true;web_.requestAuthentication();return false;}
  String capture_();
};
}
