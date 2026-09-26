// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "protocol.h"
namespace samsung_proto {
// UART 0 is the main board, UART 1 is the original display/Wi-Fi board.
// Physical forwarding is independent of application state decoding.
class InlineBridge {
 public:
  struct Stream { uint8_t data[259]{};size_t size=0;uint32_t at=0; } streams[2];
  struct Transaction {bool used=false;uint8_t source=0,counter=0,family=0,group=0,reply=0;uint32_t at=0;} pending[24];
  struct Held {uint8_t data[259]{};size_t size=0;} held[8];
  size_t held_count=0;
  bool busy=false,enabled=true;
  uint8_t own_counter=0,own_group=0,own_reply=0,next_counter=0xE1;
  uint8_t retired_reply[256]{},retired_group[256]{};
  bool seen[256]{};uint32_t counter_at[256]{};
  uint32_t last_byte=0,last_fault=0,sent_at=0,last_send=0;
  uint32_t frames[2]{},forwarded[2]{},completed=0,expired=0,errors=0,own_sent=0,own_replies=0,own_timeouts=0,overflows=0;
  size_t pending_count()const {size_t n=0;for(auto &p:pending)n+=p.used;return n;}
  void enable(bool value,uint32_t now){
    enabled=value;busy=false;held_count=0;for(auto &s:streams)s.size=0;
    for(auto &p:pending)p.used=false;last_fault=now;
  }
  bool ready(uint32_t now)const{
    return enabled&&!busy&&now>=5000&&completed>=5&&!pending_count()&&!streams[0].size&&!streams[1].size&&
      uint32_t(now-last_byte)>=80&&uint32_t(now-last_fault)>=2000&&uint32_t(now-last_send)>=300;
  }
  bool choose_counter(uint32_t now,uint8_t &counter)const{
    for(unsigned i=0;i<256;++i){uint8_t c=uint8_t(next_counter+i);if(!seen[c]||uint32_t(now-counter_at[c])>=30000){counter=c;return true;}}
    return false;
  }
  template<class Emit> bool send(uint16_t type,const Bytes &payload,uint8_t counter,uint32_t now,Emit emit){
    if(!ready(now)||((type&255)!=2&&(type&255)!=4)||(type>>8)<0x12||(type>>8)>0x14)return false;
    if(seen[counter]&&uint32_t(now-counter_at[counter])<30000)return false;
    auto p=frame(type,counter,payload);if(p.empty())return false;
    busy=true;own_counter=counter;own_group=type>>8;own_reply=uint8_t(type)+1;sent_at=last_send=now;
    retired_group[counter]=own_group;retired_reply[counter]=own_reply;seen[counter]=true;counter_at[counter]=now;next_counter=counter+1;
    ++own_sent;emit(0,p.data(),p.size(),true);return true;
  }
  template<class Emit,class Observe> void feed(unsigned c,uint8_t b,uint32_t now,Emit emit,Observe observe){
    auto &s=streams[c];last_byte=s.at=now;s.data[s.size++]=b;
    if((s.size==1&&b!=0xD0)||(s.size==2&&b!=0xC0)||(s.size==3&&b!=2)||(s.size==4&&b<12)){
      raw(c,s.data,s.size,now,emit);s.size=0;return;
    }
    if(s.size>=4&&s.size==size_t(s.data[3])+4){
      if(envelope(s.data,s.size))receive(c,s.data,s.size,now,emit,observe);else raw(c,s.data,s.size,now,emit);
      s.size=0;
    }
  }
  template<class Emit> void tick(uint32_t now,Emit emit){
    if(!enabled){for(auto &s:streams)if(s.size&&uint32_t(now-s.at)>=200)s.size=0;return;}
    for(unsigned c=0;c<2;++c){auto &s=streams[c];if(s.size&&uint32_t(now-s.at)>=200){raw(c,s.data,s.size,now,emit);s.size=0;}}
    for(auto &p:pending)if(p.used&&uint32_t(now-p.at)>=5000){p.used=false;++expired;last_fault=now;}
    if(busy&&uint32_t(now-sent_at)>=1500){busy=false;++own_timeouts;last_fault=now;release(now,emit);}
  }
 private:
  static bool envelope(const uint8_t *p,size_t n){
    if(n<16||n!=size_t(p[3])+4||n!=size_t(p[13])+16||p[n-1]!=0xE0)return false;
    uint8_t crc=0;for(size_t i=0;i<n-2;++i)crc^=p[i];return crc==p[n-2];
  }
  template<class Emit> void pass(unsigned c,const uint8_t *p,size_t n,Emit emit){if(enabled){forwarded[c]+=n;emit(1-c,p,n,false);}}
  template<class Emit> void release(uint32_t now,Emit emit){for(size_t i=0;i<held_count;++i){auto &h=held[i];track(1,h.data,now);pass(1,h.data,h.size,emit);}held_count=0;}
  template<class Emit> void raw(unsigned c,const uint8_t *p,size_t n,uint32_t now,Emit emit){
    ++errors;last_fault=now;if(busy){busy=false;++own_timeouts;release(now,emit);}pass(c,p,n,emit);
  }
  void track(unsigned c,const uint8_t *p,uint32_t now){
    uint8_t kind=p[12];
    if(c==1&&(kind==2||kind==4)){seen[p[9]]=true;counter_at[p[9]]=now;retired_reply[p[9]]=0;}
    if(kind==2||kind==4||kind==6){
      Transaction *slot=nullptr;
      for(auto &t:pending){if(t.used&&t.source==c&&t.counter==p[9]&&t.family==p[10]&&t.group==p[11]&&t.reply==kind+1){slot=&t;break;}if(!t.used&&!slot)slot=&t;}
      if(slot)*slot={true,uint8_t(c),p[9],p[10],p[11],uint8_t(kind+1),now};else last_fault=now;
    }else if(kind==3||kind==5||kind==7){
      for(auto &t:pending)if(t.used&&t.source!=c&&t.counter==p[9]&&t.family==p[10]&&t.group==p[11]&&t.reply==kind){t.used=false;++completed;return;}
    }else last_fault=now;
  }
  template<class Emit,class Observe> void receive(unsigned c,const uint8_t *p,size_t n,uint32_t now,Emit emit,Observe observe){
    ++frames[c];bool known=p[10]==0xFE||p[10]==0xFC||p[10]==0xFD;
    if(!enabled){if(c==0&&valid(p,n))observe(p,n,false);return;}
    for(unsigned i=4;i<9;++i)known=known&&p[i]==0;
    if(!known){raw(c,p,n,now,emit);return;}
    if(c==0&&p[10]==0xFE&&retired_reply[p[9]]==p[12]&&retired_group[p[9]]==p[11]){
      if(busy&&own_counter==p[9]&&own_reply==p[12]&&own_group==p[11]){
        busy=false;++own_replies;if(valid(p,n))observe(p,n,true);release(now,emit);
      }
      return; // Old own replies are not factory replies or fresh command confirmation.
    }
    if(c==1&&busy&&(p[12]==2||p[12]==4)){
      if(held_count<8){auto &h=held[held_count++];h.size=n;std::memcpy(h.data,p,n);return;}
      busy=false;++overflows;last_fault=now;release(now,emit);
    }
    track(c,p,now);pass(c,p,n,emit);
    if(c==0&&valid(p,n))observe(p,n,false);
  }
};
}
