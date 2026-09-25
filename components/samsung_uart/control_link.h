// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "protocol.h"
namespace samsung_proto {
// Readback, never a write ACK, establishes permission to control the indoor unit.
struct ControlLink {
  bool known=false, attempted=false;
  uint8_t value=0;
  uint32_t stamp=0,last_attempt=0;
  void reset(){known=false;attempted=false;}
  bool ready(uint32_t now)const{return known&&value==0x0f&&uint32_t(now-stamp)<15000;}
  bool needs_enable(uint32_t now)const{
    return known&&value==0xf0&&uint32_t(now-stamp)<10000&&
      (!attempted||uint32_t(now-last_attempt)>=30000);
  }
  void sent(uint32_t now){attempted=true;last_attempt=now;}
  void receive(const uint8_t *p,size_t n,uint32_t now){
    if(!valid(p,n)||p[10]!=0xfe||p[11]!=0x12||(p[12]!=3&&p[12]!=6))return;
    unsigned count=0;uint8_t v=0;bool good=false;
    for(size_t i=14;i<n-2;i+=2+p[i+1])if(p[i]==1){
      ++count;good=p[i+1]==1;if(good)v=p[i+2];
    }
    if(!count)return;
    known=count==1&&good&&(v==0x0f||v==0xf0);
    if(known){value=v;stamp=now;}
  }
};
}
