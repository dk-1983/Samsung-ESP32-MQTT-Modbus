// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <cstring>
namespace samsung_management {
struct Config {
 uint32_t magic=0x534d4701,baud=9600;
 uint16_t port=1883;uint8_t unit=1,rtu=0,tcp=0,mqtt=0,discovery=1;
 char host[128]{},username[65]{},password[129]{},prefix[65]="samsung-s3";
};
inline bool numeric(const char *s,uint32_t min,uint32_t max,uint32_t &value){
 if(!s||!*s)return false;uint32_t n=0;
 for(;*s;++s){if(*s<'0'||*s>'9'||n>(max-uint32_t(*s-'0'))/10)return false;n=n*10+(*s-'0');if(n>max)return false;}
 if(n<min)return false;value=n;return true;
}
inline bool host(const char *s){size_t n=strlen(s);if(!n||n>127)return false;for(;*s;++s)if(!((*s>='a'&&*s<='z')||(*s>='A'&&*s<='Z')||(*s>='0'&&*s<='9')||*s=='-'||*s=='.'))return false;return true;}
inline bool prefix(const char *s){size_t n=strlen(s);if(!n||n>64||s[0]=='/'||s[n-1]=='/')return false;for(size_t i=0;i<n;++i){char c=s[i];if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='/'))return false;if(c=='/'&&i&&s[i-1]=='/')return false;}return true;}
inline bool baud(uint32_t b){return b==9600||b==19200||b==38400||b==57600||b==115200;}
inline bool valid(const Config &c){
 if(c.magic!=0x534d4701||!baud(c.baud)||c.unit<1||c.unit>247||!c.port||c.rtu>1||c.tcp>1||c.mqtt>1||c.discovery>1)return false;
 if(!memchr(c.host,0,sizeof(c.host))||!memchr(c.username,0,sizeof(c.username))||!memchr(c.password,0,sizeof(c.password))||!memchr(c.prefix,0,sizeof(c.prefix)))return false;
 return prefix(c.prefix)&&(!c.host[0]?!c.mqtt:host(c.host));
}
}
