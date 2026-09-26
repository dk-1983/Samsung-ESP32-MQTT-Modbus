#include "components/samsung_uart/inline_bridge.h"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace samsung_proto;
struct Output{unsigned destination;Bytes bytes;bool own;};
struct Harness {
 InlineBridge bridge;Session session;std::vector<Output> out;uint32_t now=0;
 auto emit(){return [this](unsigned d,const uint8_t *p,size_t n,bool own){out.push_back({d,Bytes(p,p+n),own});};}
 auto observe(){return [this](const uint8_t *p,size_t n,bool own){if(own)session.receive(p,n,now);else session.state.update(p,n,now);};}
 void feed(unsigned c,const Bytes &p,uint32_t at){now=at;for(auto b:p)bridge.feed(c,b,now,emit(),observe());}
 void tick(uint32_t at){now=at;bridge.tick(now,emit());}
 void baseline(){for(unsigned i=0;i<5;++i){feed(1,frame(0x1202,i,{2,0}),1000+i*100);feed(0,frame(0x1203,i,{2,1,15}),1050+i*100);}out.clear();assert(bridge.ready(6000));}
 uint8_t send(uint16_t type,const Bytes &p,uint32_t at){uint8_t c;assert(bridge.choose_counter(at,c));assert(bridge.send(type,p,c,at,emit()));return c;}
};
int main(int argc,char **argv){
 {Harness h;h.baseline();auto c=h.send(0x1202,{2,0},6000);
  h.feed(1,frame(0x1202,44,{2,0}),6100);assert(h.bridge.held_count==1);
  h.feed(0,frame(0x1206,45,{2,1,15}),6120);h.feed(1,frame(0x1207,45,{2,1,15}),6140);assert(h.bridge.busy&&h.out.size()==3);
  h.feed(0,frame(0x1203,c,{2,1,15}),6200);assert(!h.bridge.busy&&h.bridge.held_count==0&&h.out.size()==4&&h.out.back().bytes==frame(0x1202,44,{2,0}));
  h.feed(0,frame(0x1203,44,{2,1,15}),6300);assert(h.bridge.ready(6450));
  uint8_t next;assert(h.bridge.choose_counter(6450,next)&&next!=c);
  auto n=h.out.size();h.feed(0,frame(0x1203,c,{2,1,0xF0}),6500);assert(h.out.size()==n&&h.session.state.value[POWER]==1);
 }
 {Harness h;h.baseline();h.session.enable(true);
  h.feed(0,frame(0x1203,60,{2,1,15,0x43,1,0x32,0x5A,1,24,0x62,1,0x12}),5800);
  Command cmd;cmd.mask=1<<FAN;cmd.value[FAN]=2;assert(h.session.accept(cmd,6000));auto wr=h.send(0x1204,command_payload(cmd),6000);
  h.feed(0,frame(0x1205,wr,{0x62,1,0x14}),6100);assert(h.session.pending);
  h.feed(0,frame(0x1206,61,{0x62,1,0x14}),6200);assert(h.session.pending);h.feed(1,frame(0x1207,61,{0x62,1,0x14}),6250);
  auto rd=h.send(0x1202,{0x62,0},6400);h.feed(0,frame(0x1203,rd,{0x62,1,0x14}),6500);assert(!h.session.pending&&h.session.result==2);
 }
 {Harness h;h.baseline();auto c=h.send(0x1202,{2,0},6000);h.feed(1,frame(0x1204,12,{0x62,1,0x12}),6100);h.tick(7500);
  assert(h.bridge.own_timeouts==1&&!h.bridge.busy&&!h.bridge.held_count);auto n=h.out.size();h.feed(0,frame(0x1203,c,{2,1,15}),7600);assert(h.out.size()==n);
 }
 {Harness h;h.baseline();h.send(0x1202,{2,0},6000);for(int i=0;i<9;++i)h.feed(1,frame(0x1202,20+i,{2,0}),6100+i);
  assert(h.bridge.overflows==1&&!h.bridge.busy&&h.bridge.held_count==0&&h.out.size()==10);
 }
 {Harness h;h.baseline();h.send(0x1202,{2,0},6000);h.feed(1,frame(0x1202,10,{2,0}),6100);h.bridge.enable(false,6150);auto n=h.out.size();
  h.feed(0,frame(0x1206,20,{2,1,0xF0}),6200);assert(h.out.size()==n&&!h.bridge.held_count&&h.session.state.value[POWER]==0);
  h.bridge.enable(true,6300);h.tick(9000);assert(h.out.size()==n);
 }
 {Harness h;h.baseline();h.feed(1,{0xD0,0xC0},5900);assert(!h.bridge.ready(6000));h.tick(6200);assert(h.out.back().bytes==Bytes({0xD0,0xC0}));
  auto bad=frame(0x1202,12,{2,0});bad.back()^=1;h.feed(0,bad,6250);assert(h.out.back().bytes==bad&&!h.bridge.ready(6300));
 }
 if(argc>1){Harness h;std::ifstream in(argv[1]);assert(in);unsigned c;uint32_t t,last=0;std::string hex;Bytes src[2],dst[2];
  while(in>>t>>c>>hex){Bytes p;for(size_t i=0;i<hex.size();i+=2)p.push_back(std::stoul(hex.substr(i,2),nullptr,16));src[c].insert(src[c].end(),p.begin(),p.end());h.feed(c,p,t);h.tick(t);last=t;}
  h.tick(last+201);for(auto &o:h.out){assert(!o.own);dst[1-o.destination].insert(dst[1-o.destination].end(),o.bytes.begin(),o.bytes.end());}
  assert(src[0]==dst[0]&&src[1]==dst[1]);std::cout<<"Replay preserved "<<src[0].size()<<"/"<<src[1].size()<<" bytes\n";
 }
 std::cout<<"PASS: production inline bridge, own readback confirmation, stock ACK forwarding, timeout, overflow, counter isolation, monitor gate\n";
}
