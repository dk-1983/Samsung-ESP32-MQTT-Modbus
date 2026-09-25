#include "components/samsung_uart/bridge.h"
#include <cassert>
#include "components/samsung_uart/control_link.h"
#include <iostream>
using namespace samsung_proto;
using namespace samsung_modbus;
Bytes full(uint16_t type=0x1203){return frame(type,7,{2,1,15,0x43,1,0x12,0x5a,1,24,0x62,1,0,0x63,1,0x12,0x44,1,0x12,0x5c,1,25});}
void ingest(Session &s,const Bytes &b,uint32_t now){s.receive(b.data(),b.size(),now);}
struct Fake:Bridge{unsigned sent=0;bool submit(const Command &c)override{if(!session.accept(c,now))return false;++sent;return true;}};
#include "test_extended.h"
#include "test_mim.h"
#include "test_management.h"
int main(){
  {
    ControlLink link;assert(!link.ready(0)&&!link.needs_enable(0));
    auto feed=[&](uint16_t type,const Bytes &payload,uint32_t t){auto b=frame(type,1,payload);link.receive(b.data(),b.size(),t);};
    feed(0x1203,{1,1,0xf0},100);assert(link.needs_enable(101));link.sent(101);
    feed(0x1205,{1,1,0x0f},102);assert(!link.ready(102)&&!link.needs_enable(102));
    feed(0x1203,{1,1,0x0f},103);assert(link.ready(103));assert(!link.ready(15103));
    feed(0x1206,{1,1,0xf0},200);assert(!link.ready(200)&&!link.needs_enable(200));
    feed(0x1203,{1,1,0xf0},30101);assert(link.needs_enable(30101));
    feed(0x1203,{1,1,15,1,1,240},30102);assert(!link.ready(30102)&&!link.needs_enable(30102));
    feed(0x1203,{1,1,15},0xfffffff0);assert(link.ready(0x10));
    link.reset();assert(!link.ready(0x10));
    assert(query_payload()[0]==1&&query_payload()[1]==0);
  }
  test_extended();
  test_mim();
  test_management();
  // Dedicated ON/OFF changes power only; no mode/target/fan/swing/preset writes.
  for(bool on:{false,true}){
    auto cmd=power_command(on);assert(cmd.mask==(1<<POWER));
    assert(command_payload(cmd)==Bytes({2,1,uint8_t(on?0x0f:0xf0)}));
    Session ps;ps.enable(true);ingest(ps,full(),100);auto old=ps.state;
    assert(ps.accept(cmd,101));assert(ps.pending);
    ingest(ps,frame(0x1205,1,{2,1,uint8_t(on?0x0f:0xf0)}),102);assert(ps.pending);
    ingest(ps,frame(0x1203,1,{2,1,uint8_t(on?0x0f:0xf0)}),103);assert(ps.result==2&&!ps.pending);
    for(unsigned f=TARGET;f<FIELD_COUNT;++f)assert(ps.state.value[f]==old.value[f]);
  }

  {
    Session ps;ps.enable(true);ingest(ps,full(),100);assert(ps.accept(power_command(false),101));
    ingest(ps,frame(0x1205,1,{2,1,0xfc}),102);assert(ps.pending&&ps.state.value[POWER]==1);
    ingest(ps,full(),103);assert(ps.pending);ps.tick(10101);assert(ps.result==3&&ps.state.value[POWER]==1);
  }
  auto b=full();assert(valid(b.data(),b.size()));
  // Reference initialization frame, independently pinned from upstream protocol notes.
  const Bytes expected={0xd0,0xc0,2,0x12,0,0,0,0,0,0,0xfe,0x12,4,6,1,1,0x0f,0x74,1,0xf0,0x64,0xe0};
  assert(frame(0x1204,0,{1,1,15,0x74,1,0xf0})==expected);
  for(size_t i=0;i<b.size();++i){auto broken=b;broken[i]^=1;assert(!valid(broken.data(),broken.size()));}
  for(size_t n=0;n<b.size();++n)assert(!valid(b.data(),n));
  assert(!valid(frame(0x1203,1,{2,8,15}).data(),19));
  Parser parser;unsigned count=0;auto rx=[&](const uint8_t *p,size_t n){assert(valid(p,n));++count;};
  for(uint8_t x:{0,0xd0,0xd0,0xc0,0,0xff})parser.push(x,rx);
  for(uint8_t v:b)parser.push(v,rx);for(uint8_t v:b)parser.push(v,rx);assert(count==2);
  auto corrupt=b;corrupt[corrupt.size()-2]^=0x80;
  for(uint8_t v:corrupt)parser.push(v,rx);for(uint8_t v:b)parser.push(v,rx);assert(count==3&&parser.bad);
  // Length disagreement is rejected before blocking the next valid frame.
  for(uint8_t v:{0xd0,0xc0,2,255,0,0,0,0,0,0,0xfe,0x12,3,0})parser.push(v,rx);
  for(uint8_t v:b)parser.push(v,rx);assert(count==4);
  // Bounded random noise must not overrun parser; arbitrary TLVs never escape their frame.
  uint32_t rng=42;for(unsigned i=0;i<100000;++i){rng=rng*1664525+1013904223;parser.push(rng>>24,rx);assert(parser.used<=259);}
  for(unsigned f=0;f<ROOM;++f)for(int v=0;v<=30;++v)if(allowed(Field(f),v))assert(decode(Field(f),encode(Field(f),v))==v);
  Session s;ingest(s,b,100);assert(s.state.fresh_mask(CORE,101));
  Command c;c.mask=1<<TARGET;c.value[TARGET]=23;assert(!s.accept(c,200)); // monitor gate
  s.enable(true);assert(s.accept(c,200));assert(!s.accept(c,201));
  ingest(s,frame(0x1205,1,{0x5a,1,23}),300);assert(s.pending&&s.state.value[TARGET]==24); // ACK != feedback
  ingest(s,frame(0x1206,1,{0x5c,1,27}),400);assert(s.pending); // unrelated status != confirmation
  ingest(s,frame(0x1203,1,{0x5a,1,23}),500);assert(!s.pending&&s.result==2);
  assert(!s.accept(c,30100)); // stale state
  ingest(s,b,31000);assert(s.accept(c,31001));s.tick(41001);assert(s.result==3&&!s.pending);
  ingest(s,b,42000);assert(s.accept(c,42001));s.enable(false);assert(!s.pending&&s.result==4);
  // A multi-field request requires each field to be freshly observed, even if cached value matched.
  s.enable(true);c.mask=(1<<TARGET)|(1<<POWER);c.value[POWER]=1;assert(s.accept(c,42002));
  ingest(s,frame(0x1203,1,{0x5a,1,23}),43000);assert(s.pending);
  ingest(s,frame(0x1206,1,{2,1,15}),43001);assert(s.result==2);
  ingest(s,frame(0x1203,1,{0x62,1,0x77}),43002);assert(!s.state.fresh(FAN,43002));
  auto duplicate=frame(0x1203,1,{0x5a,1,20,0x5a,1,21});ingest(s,duplicate,43003);assert(s.state.value[TARGET]==23);
  // uint32 uptime wrap, fresh timestamp zero.
  State z;z.update(b.data(),b.size(),0);assert(z.fresh(TARGET,1));z.update(b.data(),b.size(),0xfffffff0);assert(z.fresh(TARGET,0x10));
  Fake f;f.now=100;ingest(f.session,b,f.now);uint16_t v=0;
  assert(f.read(Table::HOLDING,58,v)==0&&v==240);assert(f.read(Table::HOLDING,3,v)==2);
  assert(f.read(Table::INPUT_REGISTER,1,v)==2);assert(f.read(Table::INPUT_REGISTER,59,v)==0&&v==250);
  Change good{Table::HOLDING,58,230};assert(f.write(&good,1)==6&&f.sent==0);
  f.session.enable(true);Change batch[]={{Table::HOLDING,58,230},{Table::HOLDING,3,1}};
  assert(f.write(batch,2)==2&&f.sent==0); // entire batch rejected before mutation
  Change bad{Table::HOLDING,58,100};assert(f.write(&bad,1)==3&&f.sent==0);
  assert(f.write(&good,1)==0&&f.sent==1);assert(f.write(&good,1)==6&&f.sent==1);
  uint8_t out[260];const uint8_t read_req[]={3,0,58,0,1};assert(pdu(f,read_req,5,out)==4&&out[3]==240);
  Bytes r={1,3,0,58,0,1};auto crc=crc16(r.data(),r.size());r.push_back(crc);r.push_back(crc>>8);
  assert(rtu(f,r.data(),r.size(),1,out)==7&&crc16(out,7)==0);
  r.back()^=1;assert(!rtu(f,r.data(),r.size(),1,out));
  const uint8_t req[]={0,42,0,0,0,6,1,3,0,58,0,1};
  assert(tcp(f,req,sizeof(req),1,out)==11&&out[1]==42&&out[10]==240);
  TcpFrame stream;for(size_t i=0;i<sizeof(req);++i)assert(stream.push(req[i])==(i+1==sizeof(req)?1:0));
  f.now=40000;assert(f.read(Table::HOLDING,58,v)==0x0b);assert(f.read(Table::INPUT_REGISTER,2480,v)==0&&v==1);
  std::cout<<"PASS: codec, corruption/resync, fuzz bounds, profile mappings, monitor gate, per-field freshness, ACK rejection, confirmation, timeout, Modbus atomic writes/RTU/TCP\n";
}
