// Minimal Arduino String/WebServer substitutes for exercising the actual page renderer.
// No networking or device operations take place in this host fixture.
#include <iostream>
#include <map>
#include <string>
#define PROGMEM
#define FPSTR(value) (value)
class String : public std::string {
 public:
 using std::string::string;
 String(const std::string &value) : std::string(value) {}
 void replace(const std::string &from, const std::string &to) {
  size_t pos=0;
  while((pos=find(from,pos))!=npos){std::string::replace(pos,from.size(),to);pos+=to.size();}
 }
};
struct HostWeb {
 String path, cookie, body;
 std::map<std::string,std::string> headers;
 String uri() const {return path;}
 String header(const char *name) const {return std::string(name)=="Cookie"?cookie:String("");}
 void sendHeader(const char *name,const char *value){headers[name]=value;}
 void send(int status,const char *type,const String &value){
  if(status!=200||std::string(type)!="text/html; charset=utf-8")throw "Unexpected response";
  body=value;
 }
};
namespace esphome::samsung_portal {
struct Portal {
 HostWeb web_;
 String token_="UI_TEST_TOKEN";
 void send_page_(const char *page);
};
}
