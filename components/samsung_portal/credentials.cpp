// SPDX-License-Identifier: GPL-3.0-or-later
#include "samsung_portal.h"
#include "PasswordsPage.h"
#include <Preferences.h>
namespace esphome::samsung_portal {
void Portal::credentials_web_(){
 web_.on("/settings",HTTP_GET,[this](){if(test_auth_())send_page_(PASSWORDS_PAGE);});
 web_.on("/settings/config",HTTP_GET,[this](){
  if(!test_auth_())return;web_.sendHeader("Cache-Control","no-store");
  web_.send(200,"application/json",credentials_ok_?"{\"credentials_ready\":true}":"{\"credentials_ready\":false}");
 });
 web_.on("/settings/config",HTTP_POST,[this](){
  if(!post_auth_())return;
  if(!credentials_ok_){web_.send(503,"text/plain","Credentials storage unavailable");return;}
  const char *names[]={"web_password","ota_password","setup_password"};
  const char *confirms[]={"web_confirm","ota_confirm","setup_confirm"};
  for(int i=0;i<web_.args();++i){String key=web_.argName(i);bool known=key=="token";
   for(int n=0;n<3;++n)known=known||key==names[n]||key==confirms[n];
   if(!known){web_.send(400,"text/plain","Unknown field");return;}
   for(int j=0;j<i;++j)if(key==web_.argName(j)){web_.send(400,"text/plain","Duplicate field");return;}
  }
  String values[3],confirm[3];samsung_credentials::Input input[3];
  for(int i=0;i<3;++i){values[i]=web_.arg(names[i]);confirm[i]=web_.arg(confirms[i]);input[i]={values[i].c_str(),values[i].length(),confirm[i].c_str(),confirm[i].length()};}
  samsung_credentials::Keys next;
  if(!samsung_credentials::apply(credentials_,input,next)){web_.send(400,"text/plain","Password confirmation or length invalid; use ASCII without spaces (web/OTA 16-64, setup 8-63)");return;}
  web_.sendHeader("Cache-Control","no-store");
  if(samsung_credentials::equal(credentials_,next)){web_.send(200,"application/json","{\"changed\":false,\"restarting\":false}");return;}
  if(ac_->session.pending||ac_->extended.pending){web_.send(409,"text/plain","AC command pending; try again shortly");return;}
  ::Preferences store;
  if(!store.begin("samsung-keys",false)||store.putBytes("keys",&next,sizeof(next))!=sizeof(next)){
   web_.send(503,"text/plain","Could not save passwords");return;
  }
  store.end();
  // Keep the current session until the response is delivered. All three services
  // load the new credentials together on the next boot.
  restart_=true;restart_at_=millis()+2000;
  web_.send(200,"application/json","{\"changed\":true,\"restarting\":true}");
 });
}
}
