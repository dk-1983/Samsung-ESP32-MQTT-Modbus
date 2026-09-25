// SPDX-License-Identifier: GPL-3.0-or-later
// Legacy register catalogue: kumy/samsung-ac/proto/consts.go, 7d9df9d.
// Names are upstream hypotheses, NOT advertised capabilities of AR24BSFCMWKNER.
#pragma once
#include "protocol.h"
namespace samsung_proto {
enum class Access:uint8_t { READ, VALUE, ACTION };
struct RegisterDef {uint8_t group,id;Access access;const char *name;};
constexpr RegisterDef EXTRA[]={
 {0x12,0x73,Access::VALUE,"Sleep flag"},
 {0x13,0x32,Access::VALUE,"Auto clean"},
 {0x13,0x75,Access::VALUE,"Ionizer SPI"},
 {0x13,0x40,Access::VALUE,"Energy setting raw"},
 {0x13,0x44,Access::ACTION,"Reset filter reminder"},
 {0x14,0xe9,Access::VALUE,"Filter interval"},
 {0x14,0xe8,Access::ACTION,"Reset usage counters"},
 {0x12,0x74,Access::READ,"Beep raw"},
 {0x12,0xf7,Access::READ,"Error bytes"},
 {0x13,0x76,Access::READ,"Outdoor temperature raw"},
 {0x13,0x77,Access::READ,"Cooling capability raw"},
 {0x13,0x78,Access::READ,"Heating capability raw"},
 {0x14,0x32,Access::READ,"Power raw"},
 {0x14,0xe0,Access::READ,"Energy raw"},
 {0x14,0xe4,Access::READ,"Runtime raw"},
 {0x14,0xe6,Access::READ,"Filter usage raw"},
 {0x14,0xf6,Access::READ,"Main version"},
 {0x14,0xf4,Access::READ,"Panel version"},
 {0x14,0xf3,Access::READ,"Outdoor version"},
 {0x14,0xf5,Access::READ,"Model code"},
 {0x14,0x39,Access::READ,"Option code"},
 {0x12,0x01,Access::READ,"Control enable"},
 {0x14,0x37,Access::READ,"WiFi status raw"},
 {0x14,0x38,Access::READ,"Internet status raw"},
 {0x12,0x63,Access::VALUE,"Legacy airflow direction"},
};
constexpr size_t EXTRA_COUNT=sizeof(EXTRA)/sizeof(EXTRA[0]);
inline bool extra_allowed(size_t i,uint16_t v){
 if(i>=EXTRA_COUNT||v>255)return false;
 switch(i){
 case 0:return v==0||v==255;
 case 1:return v==0x22||v==0x23;
 case 2:return v==15||v==240;
 case 3:return true;
 case 4:return v==0;
 case 5:return v<=4;
 case 6:return v==1;
 case 24:switch(v){case 0x12:case 0x21:case 0x31:case 0x41:case 0x51:case 0x61:case 0x71:case 0x81:case 0x82:case 0x92:case 0xa2:case 0xb2:case 0xc2:return true;}return false;
 default:return false;
 }
}
struct RegisterValue {
 std::array<uint8_t,32> bytes{};uint8_t size=0;bool known=false;uint32_t stamp=0;
 bool fresh(uint32_t now)const{return known&&uint32_t(now-stamp)<30000;}
};
struct Extended {
 std::array<RegisterValue,EXTRA_COUNT> values{};
 bool pending=false;uint8_t result=0,index=255,expected=0,counter=0;
 uint32_t started=0;Bytes last_ack;
 bool accept(size_t i,uint16_t v,uint8_t c,uint32_t now){
   if(pending||!extra_allowed(i,v))return false;
   index=i;expected=v;counter=c;started=now;pending=true;result=1;last_ack.clear();return true;
 }
 void cancel(){if(pending){pending=false;result=4;}}
 void tick(uint32_t now){if(pending&&uint32_t(now-started)>=10000){pending=false;result=3;}}
 void receive(const uint8_t *p,size_t n,uint32_t now){
   if(!valid(p,n)||p[10]!=0xfe)return;
   if(pending && p[12]==5 && p[9]==counter && p[11]==EXTRA[index].group){
     // Never convert a write reply into observed state. Even an exact action echo
     // only means acknowledged/unverified, not proof that a counter was reset.
     last_ack.assign(p+14,p+n-2);
     if(EXTRA[index].access==Access::ACTION && last_ack==Bytes{EXTRA[index].id,1,expected}){pending=false;result=5;}
   }
   if(p[12]!=3 && p[12]!=6)return;
   uint32_t seen=0;
   for(size_t at=14;at<n-2;at+=2+p[at+1])for(size_t i=0;i<EXTRA_COUNT;++i)
     if(p[11]==EXTRA[i].group&&p[at]==EXTRA[i].id){if(seen&(1u<<i))return;seen|=1u<<i;}
   for(size_t at=14;at<n-2;at+=2+p[at+1])for(size_t i=0;i<EXTRA_COUNT;++i){
     if(p[11]!=EXTRA[i].group||p[at]!=EXTRA[i].id)continue;
     auto &v=values[i];v.size=p[at+1];v.known=v.size>0&&v.size<=v.bytes.size();v.stamp=now;
     if(!v.known){v.size=0;continue;}
     v.bytes.fill(0);std::memcpy(v.bytes.data(),p+at+2,v.size);
     if(pending&&index==i&&EXTRA[i].access==Access::VALUE&&v.size==1&&v.bytes[0]==expected){pending=false;result=2;}
   }
 }
};
}
