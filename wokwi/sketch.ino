#include <Arduino.h>
#include <ESP32Servo.h>
// ESP32-WROOM DevKit; RGB catodo comun; ECHO mediante divisor 1k (arriba) + 2k (abajo)
// HC-SR04 VCC 5V, GND comun; servo 5V regulados externos, GND comun.
constexpr int TRIG=18,ECHO=19,R=25,G=26,B=27,SERVO_PIN=23;
constexpr int BTN[4]={32,33,21,22};
constexpr unsigned long DEBOUNCE=45, SAMPLE=150, COOLDOWN=9000;
Servo servo;
String input="";
enum State {ESPERA,ESCUCHANDO,PROCESANDO,RESPONDIENDO};
State state=ESPERA;
bool rawLast[4]={HIGH,HIGH,HIGH,HIGH}, stable[4]={HIGH,HIGH,HIGH,HIGH};
unsigned long changed[4]={0,0,0,0}, lastSample=0, lastDetected=0, motionEnd=0;
int nearCount=0, farCount=0;
bool armed=true;
void rgb(bool r,bool g,bool b){digitalWrite(R,r);digitalWrite(G,g);digitalWrite(B,b);}
void mode(State s){state=s; if(s==ESPERA)rgb(0,0,1);else if(s==ESCUCHANDO)rgb(0,1,1);else if(s==PROCESANDO)rgb(1,1,0);else rgb(0,1,0);}
void moveServo(){servo.write(105);motionEnd=millis()+650;}
void command(String s){s.trim();s.toUpperCase();
 if(s=="PING")Serial.println("PONG");
 else if(s=="ESPERA"||s=="REPOSO")mode(ESPERA);
 else if(s=="ESCUCHANDO")mode(ESCUCHANDO);
 else if(s=="PROCESANDO")mode(PROCESANDO);
 else if(s=="RESPONDIENDO")mode(RESPONDIENDO);
 else if(s=="MOVER_SERVO")moveServo();
 else if(s=="DEMO"){moveServo();Serial.println("DEMO_INICIADA");}
 else if(s.length())Serial.println("ERROR_COMANDO");
}
void serialPoll(){while(Serial.available()){char c=Serial.read();if(c=='\n'){command(input);input="";}else if(c!='\r'){if(input.length()<80)input+=c;else input="";}}}
void buttonsPoll(){unsigned long t=millis();for(int i=0;i<4;i++){bool v=digitalRead(BTN[i]);if(v!=rawLast[i])changed[i]=t;rawLast[i]=v;if(t-changed[i]>=DEBOUNCE&&v!=stable[i]){stable[i]=v;if(v==LOW){Serial.print("BOTON_");Serial.println(i+1);}}}}
void sensorPoll(){unsigned long t=millis();if(t-lastSample<SAMPLE)return;lastSample=t;
 digitalWrite(TRIG,LOW);delayMicroseconds(2);digitalWrite(TRIG,HIGH);delayMicroseconds(10);digitalWrite(TRIG,LOW);
 unsigned long duration=pulseIn(ECHO,HIGH,25000);if(duration==0)return;
 float cm=duration*0.0343f/2.0f;
 if(cm>=15&&cm<=80){nearCount=min(nearCount+1,4);farCount=0;}
 else if(cm>100){farCount=min(farCount+1,5);nearCount=0;}
 else {nearCount=0;}
 if(farCount>=5)armed=true;
 if(armed&&nearCount>=3&&state==ESPERA&&(lastDetected==0||t-lastDetected>COOLDOWN)){
  armed=false;lastDetected=t;Serial.println("PERSONA_DETECTADA");
 }
}
void setup(){Serial.begin(115200);pinMode(TRIG,OUTPUT);pinMode(ECHO,INPUT);digitalWrite(TRIG,LOW);
 pinMode(R,OUTPUT);pinMode(G,OUTPUT);pinMode(B,OUTPUT);
 for(int i=0;i<4;i++)pinMode(BTN[i],INPUT_PULLUP);
 servo.setPeriodHertz(50);servo.attach(SERVO_PIN,500,2400);servo.write(90);
 mode(ESPERA);delay(300);Serial.println("UTECXINA_LISTA");}
void loop(){serialPoll();buttonsPoll();sensorPoll();if(motionEnd&&((long)(millis()-motionEnd)>=0)){servo.write(90);motionEnd=0;}}
