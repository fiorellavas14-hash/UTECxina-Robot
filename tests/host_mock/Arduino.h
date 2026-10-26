// Mock minimo de la API Arduino para probar la LOGICA del firmware en el PC.
// NO es el core de ESP32: no valida que compile para ESP32.
#pragma once
#include <string>
#include <deque>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
template<class T,class U> auto min(T a,U b)->decltype(a<b?a:b){return a<b?a:b;}
struct String{
  std::string s; String(){} String(const char*c):s(c){} String(const String&)=default; String& operator=(const String&)=default;
  unsigned length()const{return s.size();}
  void trim(){size_t a=s.find_first_not_of(" \t\r\n");if(a==std::string::npos){s.clear();return;}size_t b=s.find_last_not_of(" \t\r\n");s=s.substr(a,b-a+1);}
  void toUpperCase(){for(auto&c:s)c=toupper((unsigned char)c);}
  bool operator==(const char*c)const{return s==c;}
  String& operator+=(char c){s+=c;return *this;}
};
// ---- estado simulado del hardware
extern unsigned long g_ms; extern int g_mode[64], g_out[64], g_in[64]; extern double g_dist_cm; extern std::vector<std::string> g_serial_out;
extern std::deque<char> g_serial_in;
inline unsigned long millis(){return g_ms;}
inline void delay(unsigned long ms){g_ms+=ms;}
inline void delayMicroseconds(unsigned){}
inline void pinMode(int p,int m){g_mode[p]=m; if(m==INPUT_PULLUP)g_in[p]=HIGH;}
inline void digitalWrite(int p,int v){g_out[p]=v;}
inline int digitalRead(int p){return g_in[p];}
inline unsigned long pulseIn(int,int,unsigned long timeout){
  if(g_dist_cm<=0)return 0; unsigned long us=(unsigned long)(g_dist_cm*2.0/0.0343);
  if(us>timeout)return 0; g_ms+=us/1000; return us;}
struct SerialMock{
  void begin(long b){baud=b;} long baud=0;
  int available(){return g_serial_in.size();} char read(){char c=g_serial_in.front();g_serial_in.pop_front();return c;}
  std::string cur;
  void print(const char*t){cur+=t;} void print(int v){cur+=std::to_string(v);}
  void println(const char*t){cur+=t;g_serial_out.push_back(cur);cur.clear();}
  void println(int v){cur+=std::to_string(v);g_serial_out.push_back(cur);cur.clear();}
};
extern SerialMock Serial;
