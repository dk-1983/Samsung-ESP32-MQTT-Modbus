// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "protocol.h"
#include "extended.h"
#include "modbus_core.h"
#include <algorithm>
namespace samsung_proto {
// MIM-B19N/B19NT DB68-07538A-03: zero-based PDU addresses, one indoor unit (IU0).
// Custom fields follow the complete 48-unit factory range (50..2449), not IU1.
struct Bridge : samsung_modbus::Backend {
  static constexpr uint16_t EXTRA_BASE=2450, DIAG_BASE=2475, RAW_BASE=2500;
  Session session;Extended extended;uint32_t now=0,status_count=0,last_status=0;bool seen_status=false;
  bool rtu_enabled=false,tcp_enabled=false;
  virtual bool submit(const Command &c)=0;
  virtual bool submit_extra(size_t,uint16_t){return false;}
  bool supports_function(uint8_t fc)const override{return fc==3||fc==4||fc==6||fc==16;}
  uint8_t read(samsung_modbus::Table table,uint16_t a,uint16_t &v)override{
    using samsung_modbus::Table;auto &s=session.state;Field f=POWER;
    if(table==Table::COIL)return 2;
    // Both read functions expose the same readable words; writes are separately validated.
    if(a>=EXTRA_BASE && a<EXTRA_BASE+EXTRA_COUNT){
      auto &x=extended.values[a-EXTRA_BASE];if(!x.fresh(now)||x.size!=1)return 0x0b;v=x.bytes[0];return 0;
    }
    if(a>=RAW_BASE && a<RAW_BASE+18*EXTRA_COUNT){
      auto &x=extended.values[(a-RAW_BASE)/18];unsigned offset=(a-RAW_BASE)%18;
      if(offset==17){v=x.known?std::min<uint32_t>((now-x.stamp)/1000,65535):65535;return 0;}
      if(!x.fresh(now))return 0x0b;
      v=offset==0?x.size:uint16_t(x.bytes[(offset-1)*2]<<8|x.bytes[(offset-1)*2+1]);return 0;
    }
    if(a>=DIAG_BASE && a<=DIAG_BASE+8){
      switch(a-DIAG_BASE){case 0:v=seen_status?uint16_t(std::min<uint32_t>((now-last_status)/1000,65535)):65535;break;
        case 1:v=status_count;break;case 2:v=status_count>>16;break;case 3:v=session.result;break;
        case 4:v=(s.fresh_mask(CORE,now)?1:0)|(rtu_enabled?2:0)|(tcp_enabled?4:0)|(session.enabled?8:0);break;case 5:v=session.enabled;break;
        case 6:v=extended.result;break;case 7:v=extended.index;break;case 8:v=EXTRA_COUNT;break;}return 0;
    }
    switch(a){
      // Locally derived tracking: known UART profile, not a fabricated NASA model code.
      case 50:v=s.fresh_mask(CORE,now)?7:(s.known?11:0);return 0;
      case 51:v=0xffff;return 0;
      case 52:f=POWER;break;case 53:f=MODE;break;case 54:case 2486:f=FAN;break;
      case 55:case 2484:f=SWING;break;case 58:f=TARGET;break;case 59:f=ROOM;break;
      case 2485:case 2487:f=PRESET;break;default:return 2;
    }
    if(!s.fresh(f,now))return 0x0b;
    v=s.value[f];
    if(a==53){static constexpr uint16_t map[]={0,1,4,2,3,0};v=map[v];}
    if(a==54){if(v==5)return 0x0b;v=v==4?0:v;} // Turbo has no factory IU speed code.
    if(a==55)v=v&1;
    if(a==58||a==59)v=uint16_t(int16_t(s.value[f]*10));
    if(a==2487)v=v==3;
    return 0;
  }
  uint8_t write(const samsung_modbus::Change *changes,size_t count)override{
    using samsung_modbus::Table;if(!count||count>8)return 3;Command c;
    int extra_index=-1;uint16_t extra_value=0;
    for(size_t i=0;i<count;++i){const auto &x=changes[i];Field f=POWER;int v=x.value;
      if(x.table!=Table::HOLDING)return 2;
      if(x.address>=EXTRA_BASE && x.address<EXTRA_BASE+EXTRA_COUNT){
        size_t index=x.address-EXTRA_BASE;
        if(EXTRA[index].access==Access::READ)return 2;
        if(count!=1)return 3;
        if(!extra_allowed(index,x.value))return 3;
        extra_index=int(index);extra_value=x.value;continue;
      }
      switch(x.address){
        case 52:if(v>1)continue;f=POWER;break;
        case 53:{if(v>4)continue;static constexpr int map[]={5,1,3,4,2};f=MODE;v=map[v];break;}
        case 54:if(v>3)continue;f=FAN;v=v==0?4:v;break;
        case 55:
          if(v>1)continue;
          if(!session.state.fresh(SWING,now,10000))return 0x0b;
          f=SWING;v=(session.state.value[SWING]&2)|v;break;
        case 57:
          if(v==0)continue;
          if(v!=1)return 3;
          if(count!=1)return 3;
          extra_index=4;extra_value=0;continue;
        case 58:
          if(v%10)return 3;
          f=TARGET;v/=10;break;
        case 2484:f=SWING;break;
        case 2485:f=PRESET;break;
        case 2486:f=FAN;break;
        case 2487:
          if(v>1)return 3;
          if(!session.state.fresh(PRESET,now,10000))return 0x0b;
          f=PRESET;v=v?3:(session.state.value[PRESET]==3?0:session.state.value[PRESET]);break;
        default:return 2;
      }
      if(!allowed(f,v)||(c.mask&(1<<f)))return 3;
      c.mask|=1<<f;c.value[f]=v;
    }
    if(!c.mask&&extra_index<0)return 0; // Documented ignored values: no UART write.
    if(!session.enabled||session.pending||extended.pending)return 6;
    if(!session.state.fresh_mask(CORE|c.mask,now,10000))return 0x0b;
    return (extra_index>=0?submit_extra(size_t(extra_index),extra_value):submit(c))?0:6;
  }
  bool safely_fresh()const{return session.state.fresh_mask(CORE,now,10000);}
};
}
