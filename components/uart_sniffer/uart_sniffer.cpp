#include "uart_sniffer.h"
#include <esp_system.h>
#include "esphome/components/uart/uart_component_esp_idf.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"
#include "soc/gpio_struct.h"
#include "soc/gpio_sig_map.h"
#include "esphome/components/wifi/wifi_component.h"
namespace esphome::uart_sniffer {
void IRAM_ATTR Sniffer::on_edge_(void *arg){
 auto *count=static_cast<volatile uint32_t *>(arg);*count=*count+1;
}
void Sniffer::flush_(unsigned c){auto &p=pending_[c];if(!p.size)return;p.seq=++seq_;p.channel=c;ring_[(seq_-1)%128]=p;p.size=0;}
String Sniffer::capture_(){
 uint32_t after=strtoul(web_.arg("after").c_str(),nullptr,10),first=seq_>128?seq_-127:1;
 String out;out.reserve(22000);
 out="{\"version\":\""+String(bridge_?"0.4.7-bridge":"0.4.7-sniffer")+"\",\"passive\":"+String(bridge_?"false":"true")+",\"bridge\":"+String(bridge_?"true":"false")+",\"forwarding\":"+String(bridge_&&forwarding_?"true":"false")+",\"forwarded_a\":"+String(forwarded_[0])+",\"forwarded_b\":"+String(forwarded_[1])+",\"boot_id\":"+String(boot_)+",\"uptime_ms\":"+String(millis())+",\"rx18_bytes\":"+String(bytes_[0])+",\"rx17_bytes\":"+String(bytes_[1])+",\"last_seq\":"+String(seq_)+",\"oldest_seq\":"+String(first)+",\"chunks\":[";
 bool comma=false;
 for(uint32_t i=first;i<=seq_&&i!=0;++i){if(i<=after)continue;auto &p=ring_[(i-1)%128];if(comma)out+=",";comma=true;
 out+="{\"seq\":"+String(p.seq)+",\"gpio\":"+String(rx_gpio_(p.channel))+",\"start_ms\":"+String(p.at)+",\"end_ms\":"+String(p.end)+",\"hex\":\"";
 const char *hex="0123456789ABCDEF";for(unsigned j=0;j<p.size;++j){out+=hex[p.data[j]>>4];out+=hex[p.data[j]&15];}out+="\"}";
 }out+="]}";return out;
}
void Sniffer::setup(){
 // UART pin routing alone does not fully initialize RTC-capable GPIO inputs.
 // Explicit digital input setup clears RTC mux and leaves input matrix routes intact.
 gpio_config_t rx_cfg{};
 rx_cfg.pin_bit_mask=(1ULL<<18)|(1ULL<<rx_gpio_(1));
 rx_cfg.mode=GPIO_MODE_INPUT;
 rx_cfg.pull_up_en=GPIO_PULLUP_DISABLE;rx_cfg.pull_down_en=GPIO_PULLDOWN_DISABLE;
 const auto rx_setup_error=gpio_config(&rx_cfg);
 if(rx_setup_error!=ESP_OK){this->mark_failed();return;}
 if(bridge_&&monitor_only_){
  gpio_config_t cfg{};cfg.pin_bit_mask=(1ULL<<17)|(1ULL<<factory_tx_gpio_);cfg.mode=GPIO_MODE_INPUT;
  cfg.pull_up_en=GPIO_PULLUP_DISABLE;cfg.pull_down_en=GPIO_PULLDOWN_DISABLE;
  gpio_config(&cfg);
  gpio_io_config_t a{},b{};
  if(!buses_[2]&&gpio_get_io_config(GPIO_NUM_17,&a)==ESP_OK&&gpio_get_io_config(static_cast<gpio_num_t>(factory_tx_gpio_),&b)==ESP_OK&&!a.oe&&!b.oe){
   const uint8_t probe[]={0x55,0xAA,0x00,0xFF,0x01,0x80,0x7F,0xFE};
   for(unsigned c=0;c<2;++c){
    auto *bus=static_cast<uart::IDFUARTComponent *>(buses_[c]);
    auto port=static_cast<uart_port_t>(bus->get_hw_serial_number());
    loopback_result_[c]=-2;
    if(uart_set_loop_back(port,true)!=ESP_OK)continue;
    uart_flush_input(port);bus->write_array(probe,sizeof(probe));
    uart_wait_tx_done(port,pdMS_TO_TICKS(100));
    uint8_t received[sizeof(probe)]{};
    bool ok=bus->read_array(received,sizeof(received));
    loopback_result_[c]=ok&&memcmp(probe,received,sizeof(probe))==0?1:0;
    uart_set_loop_back(port,false);uart_flush_input(port);
   }
  }
 }
 boot_=esp_random();
 const char *headers[]={"X-Samsung-Probe"};web_.collectHeaders(headers,1);
 // Observe RX edges independently of UART framing without changing pin mux/pulls.
 auto service=gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
 for(unsigned c=0;c<bus_count_();++c){
  auto pin=static_cast<gpio_num_t>(rx_gpio_(c));
  esp_err_t err=service;
  if(err==ESP_OK||err==ESP_ERR_INVALID_STATE){
   err=gpio_set_intr_type(pin,GPIO_INTR_ANYEDGE);
   if(err==ESP_OK)err=gpio_isr_handler_add(pin,on_edge_,const_cast<uint32_t *>(&rx_edges_[c]));
   if(err==ESP_OK)err=gpio_intr_enable(pin);
  }
  edge_errors_[c]=int(err);
 }
 web_.on("/",HTTP_GET,[this](){if(!auth_())return;web_.send(200,"text/html; charset=utf-8",R"HTML(<!doctype html><meta charset="utf-8"><title>Samsung UART sniffer</title><h1>Samsung UART diagnostic capture</h1><p>A: main board RX18. Factory-side GPIO numbers are shown in /diagnostics. 9600 8N1. See /capture for active mode. Bridge forwards traffic; sniffer never transmits. <a href="/capture">Raw JSON capture</a></p><pre id="out"></pre><script>let cursor=0,boot=null;async function poll(){try{let r=await fetch('/capture?after='+cursor);if(!r.ok)throw Error(r.status);let j=await r.json();if(boot!==j.boot_id){boot=j.boot_id;cursor=0;document.querySelector('pre').textContent='New boot '+boot+'\n';if(j.last_seq) {setTimeout(poll,100);return;}}let p=document.querySelector('pre');for(let c of j.chunks)p.textContent+=JSON.stringify(c)+'\n';cursor=j.last_seq;if(p.textContent.length>40000)p.textContent=p.textContent.slice(-30000);}catch(e){document.querySelector('pre').textContent+='Error '+e+'\n';}setTimeout(poll,1000);}poll();</script>)HTML");});
 web_.on("/capture",HTTP_GET,[this](){if(!auth_())return;web_.sendHeader("Cache-Control","no-store");web_.send(200,"application/json",capture_());});
 // Deliberate one-shot read for a silent bridge, never automatic initialization.
 web_.on("/probe/main-power",HTTP_POST,[this](){
  if(!auth_())return;
  if(web_.header("X-Samsung-Probe")!="read-only"){web_.send(403,"text/plain","Missing probe header");return;}
  if(!bridge_||monitor_only_||!forwarding_||probe_sent_||millis()<20000||bytes_[0]!=0||bytes_[1]>1||rx_edges_[0]||rx_edges_[1]||buses_[0]->available()||buses_[1]->available()){
   web_.send(409,"text/plain","Probe requires silent active bridge, once per boot");return;
  }
  uint8_t frame[]={0xD0,0xC0,0x02,0x0E,0,0,0,0,0,0xE1,0xFE,0x12,0x02,0x02,0x02,0x00,0,0xE0};
  for(unsigned i=0;i<sizeof(frame)-2;++i)frame[sizeof(frame)-2]^=frame[i];
  probe_sent_=true;probe_at_=millis();buses_[0]->write_array(frame,sizeof(frame));
  web_.send(200,"application/json","{\"submitted\":true,\"type\":\"FE1202\",\"field\":\"02\",\"counter\":225}");
 });
 web_.on("/diagnostics",HTTP_GET,[this](){
  if(!auth_())return;
  String out="{\"probe_sent\":"+String(probe_sent_?"true":"false")+",\"probe_at_ms\":"+String(probe_at_)+",\"monitor_only\":"+String(monitor_only_?"true":"false")+",\"loopback\":["+String(loopback_result_[0])+","+String(loopback_result_[1])+"],\"uart\":[";
  for(unsigned c=0;c<bus_count_();++c){
   auto *bus=static_cast<uart::IDFUARTComponent *>(buses_[c]);
   auto port=static_cast<uart_port_t>(bus->get_hw_serial_number());
   uint32_t baud=0;auto err=uart_get_baudrate(port,&baud);
   int rx=rx_gpio_(c);
   if(c)out+=",";
   out+="{\"edges\":"+String(uint32_t(rx_edges_[c]))+",\"edge_error\":"+String(edge_errors_[c])+",";
   out+="\"port\":"+String(unsigned(port))+",\"failed\":"+String(bus->is_failed()?"true":"false")+",\"driver_installed\":"+String(uart_is_driver_installed(port)?"true":"false")+",\"baud\":"+String(baud)+",\"baud_error\":"+String(int(err))+",\"rx_gpio\":"+String(rx)+",\"rx_level\":"+String(gpio_get_level(static_cast<gpio_num_t>(rx)))+",\"available\":"+String(bus->available())+",\"received\":"+String(bytes_[c])+",\"forwarded\":"+String(forwarded_[c])+"}";
  }
  out+="],\"rx_matrix\":[";
  const int inputs[]={U0RXD_IN_IDX,U1RXD_IN_IDX,U2RXD_IN_IDX};
  for(unsigned c=0;c<3;++c){if(c)out+=",";auto reg=GPIO.func_in_sel_cfg[inputs[c]].val;out+="{\"uart\":"+String(c)+",\"gpio\":"+String(reg&63)+",\"inverted\":"+String((reg>>6)&1)+",\"matrix\":"+String((reg>>7)&1)+"}";}
  out+="],\"pins\":[";
  const int pins[]={18,rx_gpio_(1),17,factory_tx_gpio_};
  for(unsigned c=0;c<4;++c){gpio_io_config_t conf{};int err=gpio_get_io_config(static_cast<gpio_num_t>(pins[c]),&conf);if(c)out+=",";
   out+="{\"gpio\":"+String(pins[c])+",\"error\":"+String(err)+",\"input\":"+String(conf.ie)+",\"output\":"+String(conf.oe)+",\"pullup\":"+String(conf.pu)+",\"pulldown\":"+String(conf.pd)+",\"function\":"+String(conf.fun_sel)+",\"output_signal\":"+String(conf.sig_out)+"}";
  }
  out+="]}";web_.sendHeader("Cache-Control","no-store");web_.send(200,"application/json",out);
 });

}
void Sniffer::loop(){
 for(unsigned c=0;c<bus_count_();++c){auto &p=pending_[c];uint32_t t=millis();if(p.size&&uint32_t(t-p.end)>=20)flush_(c);
  for(unsigned budget=0;budget<48&&buses_[c]->available();++budget){uint8_t v;if(!buses_[c]->read_byte(&v))break;if(c<2&&bridge_&&forwarding_){buses_[1-c]->write_byte(v);++forwarded_[c];}t=millis();if(!p.size)p.at=t;p.end=t;p.data[p.size++]=v;++bytes_[c];if(p.size==48)flush_(c);}
 }
 auto *w=wifi::global_wifi_component;
 if(!web_started_&&w&&(w->is_connected()||w->is_ap_active())){web_.begin();web_started_=true;}
 if(web_started_)web_.handleClient();
}
}
