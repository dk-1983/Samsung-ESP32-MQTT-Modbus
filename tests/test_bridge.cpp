#include "components/uart_sniffer/bridge_scheduler.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using samsung_bridge::Scheduler;
using Bytes=std::vector<uint8_t>;
struct Output{unsigned destination;Bytes bytes;bool own;};
Bytes frame(uint8_t kind,uint8_t counter,Bytes payload={2,0},uint8_t family=0xFE,uint8_t group=0x12){
 Bytes p={0xD0,0xC0,2,uint8_t(payload.size()+12),0,0,0,0,0,counter,family,group,kind,uint8_t(payload.size())};
 p.insert(p.end(),payload.begin(),payload.end());uint8_t crc=0;for(auto b:p)crc^=b;p.push_back(crc);p.push_back(0xE0);return p;
}
struct Harness{
 Scheduler s;std::vector<Output> out;
 auto emit(){return [this](unsigned d,const uint8_t*p,size_t n,bool own){out.push_back({d,Bytes(p,p+n),own});};}
 void feed(unsigned c,Bytes p,uint32_t now){for(auto b:p)s.feed(c,b,now,emit());}
 void tick(uint32_t now,bool idle=true){s.tick(now,idle,emit());}
 void baseline(){for(unsigned i=0;i<5;++i){feed(1,frame(2,i),1000+i*100);feed(0,frame(3,i,{2,1,0xF0}),1050+i*100);}assert(s.completed==5);out.clear();}
 void start(){baseline();assert(s.request(6000));tick(6000);assert(s.result==Scheduler::Result::WAITING);assert(out.size()==1&&out[0].own&&out[0].destination==0);}
};
int main(int argc,char **argv){
 {Harness h;h.start();auto counter=h.s.counter;
  h.feed(1,frame(2,42),6010);assert(h.s.held_count==1&&h.out.size()==1);
  h.feed(0,frame(6,43,{2,1,0xF0}),6020);h.feed(1,frame(7,43,{2,1,0xF0}),6030);
  assert(h.s.held_count==1&&h.s.pending()==0&&h.out.size()==3);
  h.feed(0,frame(3,counter,{2,1,0xF0}),6040);
  assert(h.s.result==Scheduler::Result::CONFIRMED&&h.s.power==0xF0&&h.s.suppressed==1);
  assert(h.out.size()==4&&h.out.back().bytes==frame(2,42)&&h.s.pending()==1);
  assert(!h.s.request(7000));
 }
 {Harness h;h.baseline();h.feed(1,frame(4,70),5900);h.s.request(6000);h.tick(6000);assert(h.s.result==Scheduler::Result::QUEUED);
  h.feed(0,frame(5,70),6050);h.tick(6100);assert(h.s.result==Scheduler::Result::QUEUED);h.tick(6140,false);assert(h.s.result==Scheduler::Result::QUEUED);h.tick(6140);assert(h.s.result==Scheduler::Result::WAITING);
 }
 {Harness h;h.start();h.feed(0,frame(3,h.s.counter+1,{2,1,0x0F}),6100);assert(h.s.result==Scheduler::Result::WAITING);
  h.feed(1,frame(4,10),6150);h.tick(7500);assert(h.s.result==Scheduler::Result::TIMEOUT&&h.s.held_count==0);
  auto n=h.out.size();h.feed(0,frame(3,h.s.counter,{2,1,0x0F}),7600);assert(h.out.size()==n&&h.s.result==Scheduler::Result::TIMEOUT);
  h.feed(1,frame(2,h.s.counter),7700);h.feed(0,frame(3,h.s.counter,{2,1,0x0F}),7800);assert(h.out.size()==n+2);
 }
 {Harness h;h.start();h.feed(0,frame(3,h.s.counter,{2,1,0x99}),6100);assert(h.s.result==Scheduler::Result::INVALID);}
 {Harness h;h.start();for(unsigned i=0;i<9;++i)h.feed(1,frame(2,i),6100+i);assert(h.s.result==Scheduler::Result::BUSY&&h.s.held_count==0&&h.out.size()==10);}
 {Harness h;h.start();h.feed(1,frame(2,9),6100);h.s.cancel(6200);h.tick(9000);assert(h.s.result==Scheduler::Result::CANCELLED&&h.s.held_count==0&&h.out.size()==1);}
 {Harness h;auto p=frame(2,1);p[p.size()-2]^=1;h.feed(1,p,100);assert(h.out.size()==1&&h.out[0].bytes==p&&h.s.errors==1);
  h.feed(0,{0xD0,0xC0},200);h.tick(400);assert(h.out.back().bytes==Bytes({0xD0,0xC0}));
  h.s.request(5000);h.tick(20000);assert(h.s.result==Scheduler::Result::BUSY);
 }
 // Replay captured chunks: passive scheduling must preserve every byte and order per direction.
 if(argc>1){Harness h;std::ifstream in(argv[1]);assert(in);unsigned c;uint32_t now,last=0;std::string hex;Bytes input[2],output[2];
  while(in>>now>>c>>hex){Bytes p;for(size_t i=0;i<hex.size();i+=2)p.push_back(std::stoul(hex.substr(i,2),nullptr,16));input[c].insert(input[c].end(),p.begin(),p.end());h.feed(c,p,now);h.tick(now,false);last=now;}
  h.tick(last+201,false);for(auto &o:h.out){assert(!o.own);auto &v=output[1-o.destination];v.insert(v.end(),o.bytes.begin(),o.bytes.end());}
  assert(input[0]==output[0]&&input[1]==output[1]);
  std::cout<<"Replay bytes "<<input[0].size()<<"/"<<input[1].size()<<", pairs "<<h.s.completed<<", pending "<<h.s.pending()<<", framing errors "<<h.s.errors<<"\n";
 }
 std::cout<<"PASS: bridge arbitration, response isolation, timeout, collision queue, notification ACK, disable, raw preservation\n";
}
