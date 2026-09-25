#pragma once
void test_mim(){
  auto init=[](Fake &f){f.now=100;ingest(f.session,full(),100);f.session.enable(true);};
  Fake f;init(f);uint16_t v=0;uint8_t out[260]{};
  assert(f.read(Table::HOLDING,50,v)==0&&v==7);
  assert(f.read(Table::INPUT_REGISTER,51,v)==0&&v==0xffff);
  for(uint8_t fc:{1,5,15}){uint8_t req[]={fc,0,0,0,1};assert(pdu(f,req,5,out)==2&&out[0]==(fc|0x80)&&out[1]==1);}
  for(uint16_t a:{0,1,2,3,4,5,56,57,60,61,62,63,64,69,70,71,78,80,81,82,99,100,124,200,649,6000,7000})
    assert(f.read(Table::HOLDING,a,v)==2); // No old-map aliases or fabricated NASA features.
  const int normalized[]={5,1,3,4,2};
  for(uint16_t mode=0;mode<5;++mode){
    Fake x;init(x);Change c{Table::HOLDING,53,mode};assert(x.write(&c,1)==0);
    auto reply=frame(0x1203,1,{0x43,1,encode(MODE,normalized[mode])});ingest(x.session,reply,101);x.now=101;
    assert(x.session.result==2&&x.read(Table::HOLDING,53,v)==0&&v==mode);
  }
  for(uint16_t fan=0;fan<4;++fan){
    Fake x;init(x);Change c{Table::HOLDING,54,fan};assert(x.write(&c,1)==0);
    auto reply=frame(0x1203,1,{0x62,1,encode(FAN,fan?fan:4)});ingest(x.session,reply,101);x.now=101;
    assert(x.session.result==2&&x.read(Table::INPUT_REGISTER,54,v)==0&&v==fan);
  }
  ingest(f.session,frame(0x1203,1,{0x62,1,0x18,0x5c,1,0xfb}),101);f.now=101;
  assert(f.read(Table::HOLDING,54,v)==0x0b&&f.read(Table::HOLDING,2486,v)==0&&v==5);
  assert(f.read(Table::INPUT_REGISTER,59,v)==0&&int16_t(v)==-50);
  for(uint16_t target:{159,160,240,245,300,301}){
    Fake x;init(x);Change c{Table::HOLDING,58,target};
    assert(x.write(&c,1)==(target==160||target==240||target==300?0:3));
  }
  // Changing vertical swing preserves the other axis in the UART's shared field.
  for(uint8_t before=0;before<4;++before)for(uint16_t on=0;on<2;++on){
    Fake x;init(x);ingest(x.session,frame(0x1203,1,{0x63,1,encode(SWING,before)}),100);
    Change c{Table::HOLDING,55,on};assert(x.write(&c,1)==0);
    ingest(x.session,frame(0x1203,1,{0x63,1,encode(SWING,(before&2)|on)}),101);x.now=101;
    assert(x.session.result==2&&x.read(Table::HOLDING,2484,v)==0&&v==((before&2)|on));
  }
  // FC16 contiguous power/mode/fan, not three independently submitted UART commands.
  Fake batch;init(batch);const uint8_t multi[]={16,0,52,0,3,6,0,1,0,3,0,1};
  assert(pdu(batch,multi,sizeof(multi),out)==5&&batch.sent==1);
  ingest(batch.session,frame(0x1203,1,{2,1,15,0x43,1,0x32}),101);assert(batch.session.pending);
  ingest(batch.session,frame(0x1203,1,{0x62,1,0x12}),102);assert(batch.session.result==2);
  Fake invalid;init(invalid);Change crossing[]={{Table::HOLDING,55,1},{Table::HOLDING,56,0}};
  assert(invalid.write(crossing,2)==2&&invalid.sent==0);
  Change ignored{Table::HOLDING,54,5};assert(invalid.write(&ignored,1)==0&&invalid.sent==0);
  struct ResetFake:Fake{size_t index=255;bool submit_extra(size_t i,uint16_t x)override{index=i;return extended.accept(i,x,1,now);}};
  ResetFake reset;init(reset);Change reset_no{Table::HOLDING,57,0},reset_yes{Table::HOLDING,57,1};
  assert(reset.write(&reset_no,1)==0&&!reset.extended.pending);
  assert(reset.write(&reset_yes,1)==0&&reset.extended.pending&&reset.index==4);
  f.now=40000;assert(f.read(Table::HOLDING,50,v)==0&&v==11);
  assert(f.read(Table::HOLDING,58,v)==0x0b);
  Fake empty;assert(empty.read(Table::HOLDING,50,v)==0&&v==0);
}
