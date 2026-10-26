#include "Arduino.h"
#include "ESP32Servo.h"
unsigned long g_ms=0; int g_mode[64],g_out[64],g_in[64]; double g_dist_cm=0; std::vector<std::string> g_serial_out; std::deque<char> g_serial_in;
SerialMock Serial; std::vector<int> g_servo_writes; int g_servo_pin=-1,g_servo_min,g_servo_max,g_servo_hz;
#include SKETCH   // el .ino sin modificar
#include <cstdlib>
static int fails=0,passes=0;
#define CHECK(c,msg) do{ if(c){passes++;printf("  OK   %s\n",msg);}else{fails++;printf("  FAIL %s\n",msg);} }while(0)
static void run(unsigned long ms){for(unsigned long i=0;i<ms;i++){g_ms++;loop();}}
static void sendcmd(const char*s){for(const char*p=s;*p;p++)g_serial_in.push_back(*p);}
static std::string lastline(){return g_serial_out.empty()?"":g_serial_out.back();}
static int count(const char*t){int n=0;for(auto&l:g_serial_out)if(l==t)n++;return n;}
static bool led(int r,int g,int b){return g_out[25]==r&&g_out[26]==g&&g_out[27]==b;}
static void press(int pin,int bounces=0){for(int i=0;i<bounces;i++){g_in[pin]=LOW;run(3);g_in[pin]=HIGH;run(3);}g_in[pin]=LOW;run(100);g_in[pin]=HIGH;run(100);}
int main(){
  printf("[1] Arranque\n");
  for(int i=0;i<64;i++){g_in[i]=HIGH;} g_dist_cm=0;
  setup();
  CHECK(Serial.baud==115200,"Serial a 115200");
  CHECK(lastline()=="UTECXINA_LISTA","envia UTECXINA_LISTA al arrancar");
  CHECK(g_mode[18]==OUTPUT&&g_mode[19]==INPUT,"GPIO18 TRIG salida, GPIO19 ECHO entrada");
  CHECK(g_mode[25]==OUTPUT&&g_mode[26]==OUTPUT&&g_mode[27]==OUTPUT,"GPIO25/26/27 salidas (RGB)");
  CHECK(g_mode[32]==INPUT_PULLUP&&g_mode[33]==INPUT_PULLUP&&g_mode[21]==INPUT_PULLUP&&g_mode[22]==INPUT_PULLUP,"botones 32/33/21/22 con INPUT_PULLUP");
  CHECK(g_servo_pin==23&&g_servo_hz==50&&g_servo_min==500&&g_servo_max==2400,"servo en GPIO23, 50 Hz, 500-2400 us");
  CHECK(!g_servo_writes.empty()&&g_servo_writes.back()==90,"servo en posicion inicial 90");
  CHECK(led(0,0,1),"modo ESPERA: LED azul (R=0,G=0,B=1)");

  printf("[2] Comandos seriales PC -> ESP32\n");
  sendcmd("PING\n");run(2);CHECK(lastline()=="PONG","PING -> PONG");
  sendcmd("ping\r\n");run(2);CHECK(lastline()=="PONG","ping en minusculas con CRLF -> PONG");
  sendcmd("PROCESANDO\n");run(2);CHECK(led(1,1,0),"PROCESANDO: LED amarillo (rojo+verde)");
  sendcmd("RESPONDIENDO\n");run(2);CHECK(led(0,1,0),"RESPONDIENDO: LED verde");
  sendcmd("ESCUCHANDO\n");run(2);CHECK(led(0,1,1),"ESCUCHANDO: LED cian (verde+azul)");
  sendcmd("ESPERA\n");run(2);CHECK(led(0,0,1),"ESPERA: LED azul");
  sendcmd("PROCESANDO\n");run(2);sendcmd("REPOSO\n");run(2);CHECK(led(0,0,1),"REPOSO equivale a ESPERA");
  size_t w0=g_servo_writes.size();
  sendcmd("MOVER_SERVO\n");run(2);
  CHECK(g_servo_writes.size()==w0+1&&g_servo_writes.back()==105,"MOVER_SERVO: servo a 105");
  run(700);CHECK(g_servo_writes.size()==w0+2&&g_servo_writes.back()==90,"el servo vuelve a 90 tras ~650 ms");
  w0=g_servo_writes.size();sendcmd("DEMO\n");run(2);
  CHECK(lastline()=="DEMO_INICIADA"&&g_servo_writes.back()==105,"DEMO: mueve servo y responde DEMO_INICIADA");
  run(700);CHECK(g_servo_writes.back()==90,"DEMO: servo regresa a 90");
  sendcmd("BAILAR\n");run(2);CHECK(lastline()=="ERROR_COMANDO","comando desconocido -> ERROR_COMANDO");
  size_t n0=g_serial_out.size();sendcmd("\n\r\n");run(2);CHECK(g_serial_out.size()==n0,"linea vacia: sin respuesta");
  sendcmd(std::string(100,'X').append("\nPING\n").c_str());run(5);CHECK(lastline()=="PONG","linea >80 caracteres no bloquea el parser");
  sendcmd("PING");run(5);n0=g_serial_out.size();sendcmd("\n");run(2);CHECK(lastline()=="PONG","comando sin \\n se procesa al llegar el \\n");

  printf("[3] Pulsadores (32,33,21,22)\n");
  const int pins[4]={32,33,21,22};
  for(int i=0;i<4;i++){int c0=count((std::string("BOTON_")+std::to_string(i+1)).c_str());press(pins[i]);
    char m[64];snprintf(m,64,"GPIO%d -> BOTON_%d (una vez)",pins[i],i+1);
    CHECK(count((std::string("BOTON_")+std::to_string(i+1)).c_str())==c0+1,m);}
  {int c0=count("BOTON_1");press(32,5);CHECK(count("BOTON_1")==c0+1,"rebotes mecanicos filtrados: un solo BOTON_1");}
  {int c0=count("BOTON_2");g_in[33]=LOW;run(2000);CHECK(count("BOTON_2")==c0+1,"pulsacion mantenida: no repite");g_in[33]=HIGH;run(100);}
  {size_t k=g_serial_out.size();g_in[21]=LOW;run(10);g_in[21]=HIGH;run(100);CHECK(g_serial_out.size()==k,"glitch de 10 ms (<45 ms) ignorado");}

  printf("[4] Sensor ultrasonico\n");
  sendcmd("ESPERA\n");run(2);g_dist_cm=200;run(1500);
  CHECK(count("PERSONA_DETECTADA")==0,"objeto a 200 cm: sin deteccion");
  g_dist_cm=50;run(2000);
  CHECK(count("PERSONA_DETECTADA")==1,"objeto a 50 cm: PERSONA_DETECTADA");
  run(3000);CHECK(count("PERSONA_DETECTADA")==1,"permanece delante: no repite");
  g_dist_cm=200;run(1500);g_dist_cm=50;run(2000);
  CHECK(count("PERSONA_DETECTADA")==1,"re-acercarse antes de 9 s: bloqueado por cooldown");
  g_dist_cm=200;run(12000);g_dist_cm=50;run(2000);
  CHECK(count("PERSONA_DETECTADA")==2,"tras alejarse y pasar 9 s: detecta de nuevo");
  g_dist_cm=200;run(12000);sendcmd("PROCESANDO\n");run(2);g_dist_cm=50;run(2000);
  CHECK(count("PERSONA_DETECTADA")==2,"fuera de ESPERA no se detecta");
  sendcmd("ESPERA\n");g_dist_cm=200;run(1500);
  {int c0=count("PERSONA_DETECTADA");g_dist_cm=10;run(2000);CHECK(count("PERSONA_DETECTADA")==c0,"objeto a 10 cm (<15): fuera de rango");
   g_dist_cm=90;run(2000);CHECK(count("PERSONA_DETECTADA")==c0,"objeto a 90 cm (>80): fuera de rango");}
  {g_dist_cm=200;run(1500);int c0=count("PERSONA_DETECTADA");g_dist_cm=79;run(2000);CHECK(count("PERSONA_DETECTADA")==c0+1,"objeto a 79 cm: dentro de rango");}
  g_dist_cm=0; run(500);
  printf("\n%d OK, %d FAIL\n",passes,fails);return fails?1:0;}
