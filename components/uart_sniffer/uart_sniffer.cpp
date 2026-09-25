#include "uart_sniffer.h"
#include <esp_system.h>
namespace esphome::uart_sniffer {
void Sniffer::flush_(unsigned c){auto &p=pending_[c];if(!p.size)return;p.seq=++seq_;p.channel=c;ring_[(seq_-1)%128]=p;p.size=0;}
String Sniffer::capture_(){
 uint32_t after=strtoul(web_.arg("after").c_str(),nullptr,10),first=seq_>128?seq_-127:1;
 String out;out.reserve(22000);
 out="{\"version\":\"0.4.7-sniffer\",\"passive\":true,\"boot_id\":"+String(boot_)+",\"uptime_ms\":"+String(millis())+",\"rx18_bytes\":"+String(bytes_[0])+",\"rx17_bytes\":"+String(bytes_[1])+",\"last_seq\":"+String(seq_)+",\"oldest_seq\":"+String(first)+",\"chunks\":[";
 bool comma=false;
 for(uint32_t i=first;i<=seq_&&i!=0;++i){if(i<=after)continue;auto &p=ring_[(i-1)%128];if(comma)out+=",";comma=true;
 out+="{\"seq\":"+String(p.seq)+",\"gpio\":"+String(p.channel?17:18)+",\"start_ms\":"+String(p.at)+",\"end_ms\":"+String(p.end)+",\"hex\":\"";
 const char *hex="0123456789ABCDEF";for(unsigned j=0;j<p.size;++j){out+=hex[p.data[j]>>4];out+=hex[p.data[j]&15];}out+="\"}";
 }out+="]}";return out;
}
void Sniffer::setup(){
 boot_=esp_random();
 web_.on("/",HTTP_GET,[this](){if(!auth_())return;web_.send(200,"text/html; charset=utf-8",R"HTML(<!doctype html><meta charset="utf-8"><title>Samsung UART sniffer</title><h1>Samsung UART: passive RX only</h1><p>GPIO18: A. GPIO17: B. 9600 8N1. No UART TX. <a href="/capture">Raw JSON capture</a></p><pre id="out"></pre><script>let cursor=0,boot=null;async function poll(){try{let r=await fetch('/capture?after='+cursor);if(!r.ok)throw Error(r.status);let j=await r.json();if(boot!==j.boot_id){boot=j.boot_id;cursor=0;document.querySelector('pre').textContent='New boot '+boot+'\n';if(j.last_seq) {setTimeout(poll,100);return;}}let p=document.querySelector('pre');for(let c of j.chunks)p.textContent+=JSON.stringify(c)+'\n';cursor=j.last_seq;if(p.textContent.length>40000)p.textContent=p.textContent.slice(-30000);}catch(e){document.querySelector('pre').textContent+='Error '+e+'\n';}setTimeout(poll,1000);}poll();</script>)HTML");});
 web_.on("/capture",HTTP_GET,[this](){if(!auth_())return;web_.sendHeader("Cache-Control","no-store");web_.send(200,"application/json",capture_());});
 web_.begin();
}
void Sniffer::loop(){
 for(unsigned c=0;c<2;++c){auto &p=pending_[c];uint32_t t=millis();if(p.size&&uint32_t(t-p.end)>=20)flush_(c);
  for(unsigned budget=0;budget<512&&buses_[c]->available();++budget){uint8_t v;if(!buses_[c]->read_byte(&v))break;t=millis();if(!p.size)p.at=t;p.end=t;p.data[p.size++]=v;++bytes_[c];if(p.size==48)flush_(c);}
 }
 web_.handleClient();
}
}
