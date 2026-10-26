#pragma once
#include "Arduino.h"
extern std::vector<int> g_servo_writes; extern int g_servo_pin; extern int g_servo_min,g_servo_max,g_servo_hz;
struct Servo{ void setPeriodHertz(int h){g_servo_hz=h;} void attach(int p,int mn,int mx){g_servo_pin=p;g_servo_min=mn;g_servo_max=mx;} void write(int a){g_servo_writes.push_back(a);} };
