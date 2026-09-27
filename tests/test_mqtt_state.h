#pragma once
#include "components/samsung_uart/mqtt_state.h"
void test_mqtt_state(){
 State state;
 auto snapshot=mqtt_state(state,100);
 assert(!snapshot.available);
 for(auto &value:snapshot.fields)assert(value=="None");
 auto frame=full();state.update(frame.data(),frame.size(),100);
 snapshot=mqtt_state(state,101);
 assert(snapshot.available&&snapshot.fields[0]=="cool"&&snapshot.fields[1]=="24"&&snapshot.fields[2]=="25");
 assert(snapshot.fields[3]=="auto"&&snapshot.fields[4]=="off"&&snapshot.fields[5]=="none");
 state.value[POWER]=0;
 assert(mqtt_state(state,101).available&&mqtt_state(state,101).fields[0]=="off");
 state.value[FAN]=5;state.value[PRESET]=8;
 assert(mqtt_state(state,101).fields[3]=="turbo"&&mqtt_state(state,101).fields[5]=="legacy_wind_3");
 // Each field expires independently. Missing data is never replaced with a default.
 state.stamp[FAN]=0;
 snapshot=mqtt_state(state,30000);
 assert(!snapshot.available&&snapshot.fields[3]=="None"&&snapshot.fields[1]=="24");
 snapshot=mqtt_state(state,30100);
 for(auto &value:snapshot.fields)assert(value=="None");
 state.update(frame.data(),frame.size(),30200);
 assert(mqtt_state(state,30201).available);
 // Partial packets and bad enum values cannot produce made-up state or index past a table.
 state.known=1<<TARGET;
 assert(!mqtt_state(state,30201).available&&mqtt_state(state,30201).fields[1]=="24");
 state.known=(1<<FIELD_COUNT)-1;state.value[FAN]=99;state.value[MODE]=99;
 snapshot=mqtt_state(state,30201);
 assert(!snapshot.available&&snapshot.fields[0]=="None"&&snapshot.fields[3]=="None");
 state.update(frame.data(),frame.size(),UINT32_MAX-10);
 assert(mqtt_state(state,20).available);
}
