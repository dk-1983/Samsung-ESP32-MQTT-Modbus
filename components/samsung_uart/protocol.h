// SPDX-License-Identifier: GPL-3.0-or-later
// Experimental D0 C0 02 Wi-Fi UART profile, documented by kumy/esphome_samsung_ac.
#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
namespace samsung_proto {
using Bytes = std::vector<uint8_t>;
inline uint8_t checksum(const uint8_t *p, size_t n) {
  uint8_t v=0; while(n--) v^=*p++; return v;
}
inline bool valid(const uint8_t *p,size_t n) {
  if(n<16 || n>259 || p[0]!=0xd0 || p[1]!=0xc0 || p[2]!=2 ||
     size_t(p[3])+4!=n || size_t(p[13])+16!=n || (p[10]!=0xfe && p[10]!=0xfc) || p[n-1]!=0xe0) return false;
  for(unsigned i=4;i<9;++i) if(p[i]) return false;
  if(checksum(p,n-2)!=p[n-2]) return false;
  for(size_t i=14;i<n-2;) {
    if(i+2>n-2 || i+2+p[i+1]>n-2) return false;
    i+=2+p[i+1];
  }
  return true;
}
inline Bytes frame(uint16_t type, uint8_t counter, const Bytes &payload) {
  if(payload.size()>243) return {};
  Bytes b={0xd0,0xc0,2,uint8_t(12+payload.size()),0,0,0,0,0,counter,0xfe,
           uint8_t(type>>8),uint8_t(type),uint8_t(payload.size())};
  b.insert(b.end(),payload.begin(),payload.end());
  b.push_back(checksum(b.data(),b.size())); b.push_back(0xe0); return b;
}
// Fixed capacity streaming parser. Corruption discards one byte, preserving nested headers.
struct Parser {
  uint8_t data[259]{}; size_t used=0; uint32_t bad=0, frames=0;
  void clear(){used=0;}
  template<class Callback> void push(uint8_t v,Callback receive) {
    if(used==sizeof(data)){std::memmove(data,data+1,--used);++bad;}
    data[used++]=v;
    for(;;){
      if(!used) return;
      if(data[0]!=0xd0 || (used>=2 && data[1]!=0xc0) || (used>=3 && data[2]!=2)) {
        std::memmove(data,data+1,--used);continue;
      }
      if(used<4)return;
      size_t n=size_t(data[3])+4;
      if(n<16 || (used>=14 && size_t(data[13])+16!=n)) {
        ++bad;std::memmove(data,data+1,--used);continue;
      }
      if(used<n)return;
      if(valid(data,n)){++frames;receive(data,n);used-=n;std::memmove(data,data+n,used);}
      else {++bad;std::memmove(data,data+1,--used);}
    }
  }
};
enum Field : uint8_t { POWER,TARGET,MODE,FAN,SWING,PRESET,ROOM,FIELD_COUNT };
constexpr uint16_t CORE=(1<<POWER)|(1<<TARGET)|(1<<MODE)|(1<<FAN);
constexpr uint8_t IDS[FIELD_COUNT]={0x02,0x5a,0x43,0x62,0x63,0x44,0x5c};
// Normalized modes: 1 cool,2 heat,3 dry,4 fan,5 auto. Fan:1 low,2 mid,3 high,4 auto,5 turbo.
inline int decode(Field field,uint8_t v){
  switch(field){
    case POWER:return v==0x0f?1:v==0xf0?0:-999;
    case TARGET:return v>=16 && v<=30?v:-999;
    case ROOM:return int8_t(v);
    case MODE:switch(v){case 0x12:return 1;case 0x42:return 2;case 0x22:return 3;case 0x32:return 4;case 0xe2:return 5;}break;
    case FAN:switch(v){case 0x12:return 1;case 0x14:return 2;case 0x16:return 3;case 0:return 4;case 0x18:return 5;}break;
    case SWING:switch(v){case 0x12:case 0xc2:return 0;case 0x92:return 1;case 0xa2:return 2;case 0xb2:return 3;}break;
    case PRESET:switch(v){case 0x12:return 0;case 0x22:return 1;case 0x42:return 2;case 0x52:return 3;case 0x32:return 4;case 0x62:return 5;case 0x82:return 6;case 0x92:return 7;case 0xa2:return 8;}break;
    default:break;
  }return -999;
}
inline uint8_t encode(Field f,int v){
  // AR24BSFCMWKNER rejects 12 for stop; C2 was confirmed by a subsequent UART read.
  const uint8_t modes[]={0,0x12,0x42,0x22,0x32,0xe2},fans[]={0,0x12,0x14,0x16,0,0x18},swings[]={0xc2,0x92,0xa2,0xb2},presets[]={0x12,0x22,0x42,0x52,0x32,0x62,0x82,0x92,0xa2};
  switch(f){case POWER:return v?0x0f:0xf0;case TARGET:return v;case MODE:return modes[v];
    case FAN:return fans[v];case SWING:return swings[v];case PRESET:return presets[v];default:return 0;}
}
inline bool allowed(Field f,int v){
  switch(f){case POWER:return v==0||v==1;case TARGET:return v>=16&&v<=30;
    case MODE:return v>=1&&v<=5;case FAN:return v>=1&&v<=5;
    case SWING:return v>=0&&v<=3;case PRESET:return v>=0&&v<=8;default:return false;}
}
struct State {
  std::array<int,FIELD_COUNT> value{};
  std::array<uint32_t,FIELD_COUNT> stamp{};
  uint16_t known=0;
  bool fresh(Field f,uint32_t now,uint32_t age=30000)const{return (known&(1<<f)) && uint32_t(now-stamp[f])<age;}
  bool fresh_mask(uint16_t mask,uint32_t now,uint32_t age=30000)const{
    for(unsigned i=0;i<FIELD_COUNT;++i)if((mask&(1<<i))&&!fresh(Field(i),now,age))return false;return true;
  }
  // Only actual read responses / unsolicited status may update state. Never write ACKs.
  uint16_t update(const uint8_t *p,size_t n,uint32_t now){
    if(!valid(p,n) || p[10]!=0xfe || p[11]!=0x12 || (p[12]!=3 && p[12]!=6))return 0;
    uint16_t changed=0,seen=0;State next=*this;
    for(size_t i=14;i<n-2;i+=2+p[i+1])for(unsigned f=0;f<FIELD_COUNT;++f)if(p[i]==IDS[f]){
      if(seen&(1<<f)) return 0; // Ambiguous duplicate register: reject whole status.
      seen|=1<<f;
      int v=p[i+1]==1?decode(Field(f),p[i+2]):-999;
      if(v==-999){next.known&=~(1<<f);continue;}
      next.value[f]=v;next.stamp[f]=now;next.known|=1<<f;changed|=1<<f;
    }
    *this=next;return changed;
  }
};
struct Command {std::array<int,FIELD_COUNT> value{};uint16_t mask=0;};
// Confirm only requested fields observed after transmission, not cached unrelated fields.
struct Session {
  State state; bool enabled=false,pending=false;Command expected;
  uint16_t observed=0;uint32_t started=0;uint8_t result=0; // 0 idle,1 pending,2 confirmed,3 timeout,4 cancelled
  void enable(bool on){enabled=on;if(!on&&pending){pending=false;result=4;} }
  bool accept(const Command &c,uint32_t now){
    if(!enabled||pending||!c.mask||c.mask&~((1<<ROOM)-1)||!state.fresh_mask(CORE,now,10000))return false;
    for(unsigned f=0;f<ROOM;++f)if((c.mask&(1<<f))&&(!allowed(Field(f),c.value[f])||!state.fresh(Field(f),now,10000)))return false;
    expected=c;pending=true;observed=0;started=now;result=1;return true;
  }
  void receive(const uint8_t *p,size_t n,uint32_t now){
    uint16_t updates=state.update(p,n,now);
    if(!pending)return;
    for(unsigned f=0;f<ROOM;++f)if(updates&(1<<f)){
      if(state.value[f]==expected.value[f])observed|=1<<f;else observed&=~(1<<f);
    }
    if((observed&expected.mask)==expected.mask && state.fresh_mask(expected.mask,now,10000)){pending=false;result=2;}
  }
  void tick(uint32_t now){if(pending&&uint32_t(now-started)>=10000){pending=false;result=3;}}
};
inline Bytes command_payload(const Command &c){
  Bytes p;for(unsigned f=0;f<ROOM;++f)if(c.mask&(1<<f)){
    if(!allowed(Field(f),c.value[f]))return {};p.insert(p.end(),{IDS[f],1,encode(Field(f),c.value[f])});
  }
  return p;
}
inline Bytes query_payload(){Bytes p;for(uint8_t id:IDS)p.insert(p.end(),{id,0});return p;}
}
