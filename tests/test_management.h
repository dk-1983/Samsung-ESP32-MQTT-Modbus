#pragma once
#include "components/samsung_portal/settings_model.h"
#include "components/samsung_portal/update_model.h"
void test_management(){
 using namespace samsung_management;
 Config c;assert(valid(c)&&!c.mqtt&&!c.rtu&&!c.tcp&&c.baud==9600);
 c.mqtt=1;assert(!valid(c));strcpy(c.host,"broker.local");assert(valid(c));
 assert(!host("mqtt://broker")&&!host("a b")&&!prefix("a/#")&&!prefix("/a")&&!prefix("a//b")&&prefix("samsung/test_1"));
 c.unit=0;assert(!valid(c));c.unit=247;c.baud=115200;assert(valid(c));c.baud=12345;assert(!valid(c));
 c.baud=9600;memset(c.password,'x',sizeof(c.password));assert(!valid(c));
 uint32_t n=0;assert(numeric("247",1,247,n)&&n==247);
 for(auto s:{"","0","248","-1","1x","999999999999999999"})assert(!numeric(s,1,247,n));
 assert(haier_management::newer("0.4.0","0.3.0"));assert(!haier_management::newer("0.4.0","0.4.0"));assert(!haier_management::newer("0.4.0-beta","0.3.0"));
 Fake f;f.now=100;ingest(f.session,full(),100);uint16_t flags=0;
 assert(f.read(Table::INPUT_REGISTER,2479,flags)==0&&flags==1);
 f.rtu_enabled=true;assert(f.read(Table::INPUT_REGISTER,2479,flags)==0&&flags==3);
 f.tcp_enabled=true;assert(f.read(Table::INPUT_REGISTER,2479,flags)==0&&flags==7);
}
