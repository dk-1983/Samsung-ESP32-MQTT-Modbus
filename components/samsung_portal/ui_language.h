#pragma once
#include <string>
namespace samsung_management {
// English is the default. Only an exact, supported preference selects Russian.
inline bool russian_ui(const std::string &cookie){
 size_t start=0;
 while(start<cookie.size()){
  size_t end=cookie.find(';',start);if(end==std::string::npos)end=cookie.size();
  auto item=cookie.substr(start,end-start);
  auto first=item.find_first_not_of(" \t"),last=item.find_last_not_of(" \t");
  if(first!=std::string::npos){
   item=item.substr(first,last-first+1);
   if(item.rfind("samsung_ui_lang=",0)==0)return item=="samsung_ui_lang=ru";
  }
  start=end+1;
 }
 return false;
}
}
