// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <cstring>
#include <cstddef>
namespace samsung_credentials {
// Preserve the provisioned 0.4.0+ NVS binary layout.
struct Keys {uint32_t magic=0x534b5901;char web[65]{},ota[65]{},setup[64]{};};
static_assert(sizeof(Keys)==200,"NVS credentials layout changed");
inline bool valid(const Keys &k){
 return k.magic==0x534b5901&&memchr(k.web,0,sizeof(k.web))&&memchr(k.ota,0,sizeof(k.ota))&&
  memchr(k.setup,0,sizeof(k.setup))&&strlen(k.web)>=8&&strlen(k.ota)>=16&&strlen(k.setup)>=8;
}
inline bool password(const char *s,size_t n,size_t min,size_t max){
 if(n<min||n>max)return false;
 for(size_t i=0;i<n;++i)if(static_cast<unsigned char>(s[i])<33||static_cast<unsigned char>(s[i])>126)return false;
 return true;
}
struct Input {const char *value;size_t size;const char *confirm;size_t confirm_size;};
// No partial update: validation of every field precedes committing the candidate.
inline bool apply(const Keys &old,const Input (&input)[3],Keys &out){
 if(!valid(old))return false;Keys next=old;
 char *dest[]={next.web,next.ota,next.setup};
 for(size_t i=0;i<3;++i){const auto &x=input[i];
  if(!x.size&&!x.confirm_size)continue;
  if(x.size!=x.confirm_size||!password(x.value,x.size,i==2?8:16,i==2?63:64)||memcmp(x.value,x.confirm,x.size))return false;
  memcpy(dest[i],x.value,x.size);dest[i][x.size]=0;
 }
 out=next;return true;
}
inline bool equal(const Keys &a,const Keys &b){return !strcmp(a.web,b.web)&&!strcmp(a.ota,b.ota)&&!strcmp(a.setup,b.setup);}
}
