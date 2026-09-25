#pragma once
void test_extended(){
  Extended e;assert(!e.accept(7,15,1,0));assert(!e.accept(25,0,1,0));
  assert(!e.accept(1,15,1,0));assert(e.accept(1,0x22,7,0));
  auto receive=[&](const Bytes &b,uint32_t now){e.receive(b.data(),b.size(),now);};
  receive(frame(0x1305,7,{0x32,1,0x22}),1);
  assert(e.pending&&!e.values[1].known&&!e.last_ack.empty());
  receive(frame(0x1203,7,{0x32,1,0x22}),2);assert(e.pending); // wrong group
  receive(frame(0x1303,8,{0x32,1,0x23}),3);assert(e.pending);
  receive(frame(0x1303,9,{0x32,1,0x22}),4);assert(!e.pending&&e.result==2);
  assert(e.values[1].fresh(5)&&!e.values[1].fresh(30004));
  receive(frame(0x1303,1,{0x32,1,0x23,0x32,1,0x22}),5);assert(e.values[1].stamp==4);
  receive(frame(0x1203,2,{0xf7,4,0,1,0,2}),6);assert(e.values[8].size==4&&e.values[8].bytes[3]==2);
  assert(e.accept(4,0,10,10));receive(frame(0x1305,11,{0x44,1,0}),11);assert(e.pending);
  receive(frame(0x1305,10,{0x44,1,0xfc}),12);assert(e.pending); // FC is not successful reset
  receive(frame(0x1305,10,{0x44,1,0}),13);assert(!e.pending&&e.result==5&&!e.values[4].known);
  assert(e.accept(0,0,12,0xfffffff0));e.tick(0x20);assert(e.pending);e.tick(0x3000);assert(e.result==3);
  assert(e.accept(0,255,13,15000));e.cancel();assert(!e.pending&&e.result==4);
  auto fc=frame(0x1203,1,{2,1,15});fc[10]=0xfc;fc[fc.size()-2]=checksum(fc.data(),fc.size()-2);
  assert(valid(fc.data(),fc.size()));State state;assert(state.update(fc.data(),fc.size(),0)==0&&!state.known);
  assert(e.accept(1,0x22,14,16000));auto service=frame(0x1303,14,{0x32,1,0x22});service[10]=0xfc;service[service.size()-2]=checksum(service.data(),service.size()-2);
  receive(service,16001);assert(e.pending);e.cancel();
  struct ExtraFake:Fake{bool submit_extra(size_t i,uint16_t v)override{return extended.accept(i,v,1,now);}};
  ExtraFake f;f.now=100;ingest(f.session,full(),100);f.session.enable(true);
  uint16_t value=0;assert(f.read(Table::HOLDING,2451,value)==0x0b);
  Change mixed[]={{Table::HOLDING,2451,0x22},{Table::HOLDING,58,240}};
  assert(f.write(mixed,2)==3&&!f.extended.pending);
  Change ro{Table::HOLDING,2458,0};assert(f.write(&ro,1)==2);
  Change good{Table::HOLDING,2451,0x22};assert(f.write(&good,1)==0&&f.extended.pending);
  Change core{Table::HOLDING,54,1};assert(f.write(&core,1)==6);
  auto update=frame(0x1303,1,{0x32,1,0x22});f.extended.receive(update.data(),update.size(),101);f.now=102;
  assert(f.read(Table::HOLDING,2451,value)==0&&value==0x22);
  assert(f.read(Table::INPUT_REGISTER,2518,value)==0&&value==1);
  assert(f.read(Table::INPUT_REGISTER,2519,value)==0&&value==0x2200);
  assert(f.read(Table::INPUT_REGISTER,2534,value)==0&&value==0);
  f.now=40000;assert(f.read(Table::INPUT_REGISTER,2518,value)==0x0b);
  assert(f.read(Table::INPUT_REGISTER,2535,value)==0&&value==39);
  f.now=102;f.session.enable(false);assert(f.write(&good,1)==6);
  // Known rejection/unsupported bytes stay raw; selectors never reinterpret them as true.
  update=frame(0x1303,1,{0x32,1,0xfc});f.extended.receive(update.data(),update.size(),103);
  assert(f.extended.values[1].bytes[0]==0xfc&&!extra_allowed(1,0xfc));
  assert(extra_allowed(24,0xb2)&&!extra_allowed(24,0xff));
  for(int i=0;i<=8;++i)assert(decode(PRESET,encode(PRESET,i))==i);
  assert(encode(SWING,0)==0xc2&&decode(SWING,0xc2)==0&&decode(SWING,0x12)==0);
  Session swing;ingest(swing,full(),100);swing.enable(true);
  Command stop;stop.mask=1<<SWING;stop.value[SWING]=0;assert(swing.accept(stop,101));
  assert(command_payload(stop)==Bytes({0x63,1,0xc2}));
  ingest(swing,frame(0x1205,1,{0x63,1,0xfe}),102);assert(swing.pending);
  ingest(swing,frame(0x1203,2,{0x63,1,0xc2}),103);assert(!swing.pending&&swing.result==2);
}
