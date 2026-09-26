#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace samsung_bridge {
// One manually requested operation per boot: power read or guarded fan test.
// Directions identify the receiver: 0 main board, 1 factory Wi-Fi module.
class Scheduler {
 public:
  enum class Result { IDLE, QUEUED, WAITING, CONFIRMED, TIMEOUT, BUSY, CANCELLED, INVALID, REJECTED };
  enum class Phase { POWER, PREFLIGHT, WRITE_FAN, VERIFY_FAN };
  Phase phase=Phase::POWER;
  uint8_t retired[256]{}, mode=0, fan=0, permission=0, target_fan=0, before_fan=0;
  uint32_t preflight_at=0;
  bool write_ack=false, preflight_changed=false;
  struct Stream { uint8_t data[259]{}; size_t size=0; uint32_t at=0; } stream[2];
  struct Transaction { bool used=false; uint8_t source=0,counter=0,family=0,group=0,reply=0; uint32_t at=0; } transactions[24];
  struct Held { uint8_t data[259]{}; size_t size=0; } held[8];
  size_t held_count=0;
  Result result=Result::IDLE;
  uint32_t valid[2]{}, errors=0, completed=0, expired=0, suppressed=0;
  uint32_t last_byte=0, last_fault=0, requested_at=0, sent_at=0;
  uint32_t counter_at[256]{};
  bool counter_seen[256]{}, used=false, tombstone=false;
  uint8_t counter=0, power=0;
  size_t pending() const { size_t n=0; for(auto &t:transactions)n+=t.used; return n; }
  const char *status() const {
    switch(result){case Result::IDLE:return "idle";case Result::QUEUED:return "queued";
    case Result::WAITING:return "waiting";case Result::CONFIRMED:return "confirmed";
    case Result::TIMEOUT:return "timeout";case Result::BUSY:return "busy";
    case Result::CANCELLED:return "cancelled";case Result::REJECTED:return "rejected";default:return "invalid_response";}
  }
  bool request(uint32_t now) {
    if(used)return false;
    used=true;result=Result::QUEUED;requested_at=now;return true;
  }
  bool request_fan(uint32_t now) {
    if(!request(now))return false;
    phase=Phase::PREFLIGHT;return true;
  }
  const char *phase_name()const{
    switch(phase){case Phase::POWER:return "power_read";case Phase::PREFLIGHT:return "preflight";
    case Phase::WRITE_FAN:return "fan_write";default:return "fan_readback";}
  }
  // Disable drops buffered data, matching the forwarding switch's no-replay policy.
  void cancel(uint32_t now) {
    if(result==Result::WAITING||result==Result::QUEUED)result=Result::CANCELLED;
    for(auto &s:stream)s.size=0;
    for(auto &t:transactions)t.used=false;
    held_count=0;last_fault=now;
  }
  static bool frame_valid(const uint8_t *p,size_t n) {
    if(n<16||n>259||p[0]!=0xD0||p[1]!=0xC0||p[2]!=2||n!=size_t(p[3])+4||n!=size_t(p[13])+16||p[n-1]!=0xE0)return false;
    uint8_t crc=0;for(size_t i=0;i<n-2;++i)crc^=p[i];
    return crc==p[n-2];
  }
  template<class Emit> void feed(unsigned c,uint8_t b,uint32_t now,Emit emit) {
    last_byte=now;auto &s=stream[c];s.at=now;s.data[s.size++]=b;
    if((s.size==1&&b!=0xD0)||(s.size==2&&b!=0xC0)||(s.size==3&&b!=2)||(s.size==4&&b<12)){
      raw(c,s.data,s.size,now,emit);s.size=0;return;
    }
    if(s.size>=4&&s.size==size_t(s.data[3])+4){
      if(frame_valid(s.data,s.size))frame(c,s.data,s.size,now,emit);
      else raw(c,s.data,s.size,now,emit);
      s.size=0;
    }
  }
  template<class Emit> void tick(uint32_t now,bool uart_idle,Emit emit) {
    for(unsigned c=0;c<2;++c){auto &s=stream[c];if(s.size&&uint32_t(now-s.at)>=200){raw(c,s.data,s.size,now,emit);s.size=0;}}
    for(auto &t:transactions)if(t.used&&uint32_t(now-t.at)>=5000){t.used=false;++expired;last_fault=now;}
    if(result==Result::WAITING&&uint32_t(now-sent_at)>=1500){result=Result::TIMEOUT;last_fault=now;release(now,emit);}
    if(result!=Result::QUEUED)return;
    if(uint32_t(now-requested_at)>=15000){result=Result::BUSY;return;}
    if(!uart_idle||now<5000||uint32_t(now-last_byte)<80||uint32_t(now-last_fault)<2000||stream[0].size||stream[1].size||pending()||completed<5)return;
    bool found=false;
    for(unsigned i=0;i<256;++i){uint8_t candidate=uint8_t(0xE1+i);if(!retired[candidate]&&(!counter_seen[candidate]||uint32_t(now-counter_at[candidate])>=30000)){counter=candidate;found=true;break;}}
    if(!found)return;
    if(phase==Phase::WRITE_FAN&&(preflight_changed||uint32_t(now-preflight_at)>3000)){
      result=Result::REJECTED;return;
    }
    uint8_t p[32]={0xD0,0xC0,2,0,0,0,0,0,0,counter,0xFE,0x12,2,0};
    size_t payload=0;
    if(phase==Phase::POWER){p[14]=2;p[15]=0;payload=2;}
    else if(phase==Phase::WRITE_FAN){p[12]=4;p[14]=0x62;p[15]=1;p[16]=target_fan;payload=3;}
    else {const uint8_t fields[]={1,0,2,0,0x43,0,0x62,0};std::memcpy(p+14,fields,sizeof(fields));payload=sizeof(fields);}
    size_t n=payload+16;p[3]=uint8_t(n-4);p[13]=uint8_t(payload);p[n-1]=0xE0;
    for(size_t i=0;i<n-2;++i)p[n-2]^=p[i];
    result=Result::WAITING;sent_at=now;tombstone=true;
    retired[counter]|=uint8_t(1u<<(p[12]+1));emit(0,p,n,true);
  }
 private:
  static bool field(const uint8_t *p,size_t n,uint8_t id,uint8_t &value){
    unsigned count=0;bool good=false;uint8_t found=0;
    for(size_t i=14;i<n-2;){
      if(i+2>n-2||i+2+p[i+1]>n-2)return false;
      if(p[i]==id){++count;good=p[i+1]==1;if(good)found=p[i+2];}
      i+=2+p[i+1];
    }
    if(count!=1||!good)return false;value=found;return true;
  }
  template<class Emit> void release(uint32_t now,Emit emit){
    for(size_t i=0;i<held_count;++i){auto &h=held[i];track(1,h.data,now);emit(0,h.data,h.size,false);}held_count=0;
  }
  template<class Emit> void raw(unsigned c,const uint8_t *p,size_t n,uint32_t now,Emit emit){
    ++errors;last_fault=now;
    if(result==Result::WAITING){result=Result::INVALID;release(now,emit);}
    emit(1-c,p,n,false);
  }
  void track(unsigned c,const uint8_t *p,uint32_t now){
    uint8_t kind=p[12];
    if(c==1&&kind==4&&phase==Phase::WRITE_FAN&&result==Result::QUEUED)preflight_changed=true;
    if(c==1&&(kind==2||kind==4)){
      counter_seen[p[9]]=true;counter_at[p[9]]=now;
      // Once the factory reuses our counter, its replies must pass normally.
      retired[p[9]]=0;
      if(tombstone&&p[9]==counter)tombstone=false;
    }
    if(kind==2||kind==4||kind==6){
      Transaction *slot=nullptr;
      for(auto &t:transactions){
        if(t.used&&t.source==c&&t.counter==p[9]&&t.family==p[10]&&t.group==p[11]&&t.reply==kind+1){slot=&t;break;}
        if(!t.used&&!slot)slot=&t;
      }
      if(slot)*slot={true,uint8_t(c),p[9],p[10],p[11],uint8_t(kind+1),now};
      else last_fault=now;
    }else if(kind==3||kind==5||kind==7){
      for(auto &t:transactions)if(t.used&&t.source!=c&&t.counter==p[9]&&t.family==p[10]&&t.group==p[11]&&t.reply==kind){t.used=false;++completed;return;}
    }else last_fault=now;
  }
  template<class Emit> void frame(unsigned c,const uint8_t *p,size_t n,uint32_t now,Emit emit){
    ++valid[c];
    bool known=p[10]==0xFE||p[10]==0xFC||p[10]==0xFD;
    for(size_t i=4;i<9;++i)known=known&&p[i]==0;
    if(!known){raw(c,p,n,now,emit);return;}
    if(c==0&&p[10]==0xFE&&p[11]==0x12&&(p[12]==3||p[12]==5)&&
       (retired[p[9]]&(1u<<p[12]))){
      ++suppressed;
      if(result==Result::WAITING&&p[9]==counter){
        if(phase==Phase::POWER){
          if(n==19&&p[13]==3&&p[14]==2&&p[15]==1&&(p[16]==0x0F||p[16]==0xF0)){power=p[16];result=Result::CONFIRMED;}
          else result=Result::INVALID;
        }else if(phase==Phase::WRITE_FAN){
          uint8_t v=0;
          if(p[12]==5&&field(p,n,0x62,v)&&v==target_fan){write_ack=true;phase=Phase::VERIFY_FAN;result=Result::QUEUED;requested_at=now;}
          else result=Result::REJECTED;
        }else {
          bool ok=p[12]==3&&field(p,n,1,permission)&&field(p,n,2,power)&&field(p,n,0x43,mode)&&field(p,n,0x62,fan);
          if(!ok)result=Result::INVALID;
          else if(phase==Phase::PREFLIGHT){
            // This bench command is only for a running fan-only unit with fixed speed.
            if(permission!=0x0F||power!=0x0F||mode!=0x32||(fan!=0x12&&fan!=0x14&&fan!=0x16&&fan!=0x18))result=Result::REJECTED;
            else {before_fan=fan;target_fan=fan==0x18?0x16:uint8_t(fan+2);preflight_at=now;phase=Phase::WRITE_FAN;result=Result::QUEUED;requested_at=now;}
          }else result=(power==0x0F&&mode==0x32&&fan==target_fan)?Result::CONFIRMED:Result::REJECTED;
        }
        release(now,emit);
      }
      return;
    }
    // A state change between preflight and write invalidates the decision to transmit.
    if(c==0&&phase==Phase::WRITE_FAN&&result==Result::QUEUED&&p[10]==0xFE&&p[11]==0x12&&(p[12]==3||p[12]==6)){
      const uint8_t ids[]={1,2,0x43,0x62},expected[]={permission,power,mode,before_fan};
      for(unsigned i=0;i<4;++i){uint8_t v=0;if(field(p,n,ids[i],v)&&v!=expected[i])preflight_changed=true;}
    }
    if(c==1&&result==Result::WAITING&&(p[12]==2||p[12]==4)){
      if(held_count<8){auto &h=held[held_count++];h.size=n;std::memcpy(h.data,p,n);return;}
      result=Result::BUSY;last_fault=now;release(now,emit);
    }
    track(c,p,now);emit(1-c,p,n,false);
  }
};
}
