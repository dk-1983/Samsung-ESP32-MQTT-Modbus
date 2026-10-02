#pragma once
#include "components/samsung_portal/settings_model.h"
#include "components/samsung_portal/credentials_model.h"
#include "components/samsung_portal/update_model.h"
#include "components/samsung_portal/boot_events.h"
#include "components/samsung_portal/ui_language.h"
void test_management(){
 using namespace samsung_management;
 assert(!russian_ui("")&&!russian_ui("samsung_ui_lang=en"));
 assert(russian_ui("samsung_ui_lang=ru")&&russian_ui("other=1; samsung_ui_lang=ru; extra=2"));
 assert(!russian_ui("other_samsung_ui_lang=ru")&&!russian_ui("samsung_ui_lang=rus"));
 assert(!russian_ui("other=ru")&&!russian_ui("samsung_ui_lang=de"));
 assert(!russian_ui("samsung_ui_lang=")&&!russian_ui("samsung_ui_lang=RU"));
 assert(!russian_ui("samsung_ui_lang=ru-extra")&&!russian_ui("samsung_ui_lang=%72%75"));
 assert(russian_ui(" ;\t samsung_ui_lang=ru ; unrelated=value"));
 assert(!russian_ui("unrelated=samsung_ui_lang=ru"));
 assert(!russian_ui("samsung_ui_lang=en; samsung_ui_lang=ru"));
 {
  BootEvents b;
  assert(b.poll(0,true,false,false)==BootEvents::NONE);
  assert(b.poll(10000,true,true,false)==BootEvents::NONE);
  assert(b.poll(14999,true,true,false)==BootEvents::NONE);
  assert(b.poll(15000,true,true,false)==BootEvents::BOOT_READY);
  assert(b.poll(16000,true,true,false)==BootEvents::BOOT_READY); // Retry failed publish.
  b.published(BootEvents::BOOT_READY,false);
  assert(b.poll(17000,true,true,true)==BootEvents::NONE);
  assert(b.poll(22000,true,true,true)==BootEvents::AC_CONNECTION_RESTORED);
  b.published(BootEvents::AC_CONNECTION_RESTORED,true);
  assert(b.poll(23000,true,true,true)==BootEvents::NONE);
  assert(b.poll(24000,false,true,true)==BootEvents::NONE);
  assert(b.poll(25000,true,true,true)==BootEvents::NONE);
  assert(b.poll(30000,true,true,true)==BootEvents::MQTT_RECONNECTED);
  b.published(BootEvents::MQTT_RECONNECTED,true);
  assert(b.poll(31000,true,true,true)==BootEvents::NONE);
  BootEvents trial;
  assert(trial.poll(0,true,true,true)==BootEvents::NONE);
  assert(trial.poll(4000,true,false,true)==BootEvents::NONE);
  assert(trial.poll(5000,true,true,true)==BootEvents::NONE);
  assert(trial.poll(9999,true,true,true)==BootEvents::NONE);
  assert(trial.poll(10000,true,true,true)==BootEvents::BOOT_READY);
  BootEvents wrap;
  assert(wrap.poll(UINT32_MAX-2000,true,true,true)==BootEvents::NONE);
  assert(wrap.poll(3000,true,true,true)==BootEvents::BOOT_READY);
 }
 Config c;assert(valid(c)&&!c.mqtt&&!c.rtu&&!c.tcp&&c.baud==9600);
 c.mqtt=1;assert(!valid(c));strcpy(c.host,"broker.local");assert(valid(c));
 assert(!host("mqtt://broker")&&!host("a b")&&!prefix("a/#")&&!prefix("/a")&&!prefix("a//b")&&prefix("samsung/test_1"));
 c.unit=0;assert(!valid(c));c.unit=247;c.baud=115200;assert(valid(c));c.baud=12345;assert(!valid(c));
 c.baud=9600;memset(c.password,'x',sizeof(c.password));assert(!valid(c));
 uint32_t n=0;assert(numeric("247",1,247,n)&&n==247);
 for(auto s:{"","0","248","-1","1x","999999999999999999"})assert(!numeric(s,1,247,n));
 assert(samsung_update::newer("0.4.0","0.3.0"));assert(!samsung_update::newer("0.4.0","0.4.0"));assert(!samsung_update::newer("0.4.0-beta","0.3.0"));
 {
 using namespace samsung_credentials;
 Keys old;strcpy(old.web,"legacy08");strcpy(old.ota,"old-ota-password-123");strcpy(old.setup,"setup123");assert(valid(old));
 Input inputs[3]={{"",0,"",0},{"",0,"",0},{"",0,"",0}};Keys out;
 assert(apply(old,inputs,out)&&equal(old,out));
 const char *next="new-web-password-123";inputs[0]={next,strlen(next),next,strlen(next)};
 assert(apply(old,inputs,out)&&!strcmp(out.web,next)&&!strcmp(out.ota,old.ota)&&!strcmp(out.setup,old.setup));
 Keys untouched=out;inputs[2]={"short",5,"short",5};assert(!apply(old,inputs,out)&&equal(out,untouched));
 inputs[2]={"",0,"",0};inputs[0].confirm="bad";inputs[0].confirm_size=3;assert(!apply(old,inputs,out));
 assert(!password("abc defghijklmnop",16,16,64));
 const char embedded[]={ 'a',0,'b' };assert(!password(embedded,3,1,64));
 std::string boundary(64,'x');assert(password(boundary.c_str(),64,16,64)&&!password(boundary.c_str(),64,8,63));
 boundary+='x';assert(!password(boundary.c_str(),65,16,64));
 Keys broken=old;memset(broken.web,'x',sizeof(broken.web));assert(!valid(broken));
 }
 Fake f;f.now=100;ingest(f.session,full(),100);uint16_t flags=0;
 assert(f.read(Table::INPUT_REGISTER,2479,flags)==0&&flags==1);
 f.rtu_enabled=true;assert(f.read(Table::INPUT_REGISTER,2479,flags)==0&&flags==3);
 f.tcp_enabled=true;assert(f.read(Table::INPUT_REGISTER,2479,flags)==0&&flags==7);
}
