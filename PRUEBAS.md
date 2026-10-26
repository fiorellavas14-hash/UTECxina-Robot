# Guía de pruebas — UTECxina

Marca con ✔ lo que compruebes. Las secciones A y B son idénticas en Wokwi y en placa real: en Wokwi escribes los comandos en la
caja del monitor serie; en físico, en el Monitor Serie de Arduino IDE (115200, con terminación *Nueva línea*; **ciérralo antes de abrir Python**).

> Las pruebas del firmware en PC (`tests/`) ya cubren la lógica de A, B y C. Lo que falta por comprobar es que Wokwi / la placa real
> se comporten igual; por eso esta guía está pensada para que la ejecutes tú.

## A. Arranque y comandos seriales

| # | Acción | Resultado esperado |
|---|---|---|
| A1 | Play / reiniciar la placa | Salida `UTECXINA_LISTA`. LED **azul**. Servo en reposo (90°). |
| A2 | Enviar `PING` | `PONG` |
| A3 | Enviar `PROCESANDO` | LED **amarillo** (rojo + verde). Sin texto de respuesta. |
| A4 | Enviar `RESPONDIENDO` | LED **verde** |
| A5 | Enviar `ESCUCHANDO` | LED **cian** (verde + azul) |
| A6 | Enviar `ESPERA` (o `REPOSO`) | LED **azul** |
| A7 | Enviar `MOVER_SERVO` | El servo gira a 105° y vuelve a 90° en ~0,65 s. Sin texto de respuesta. |
| A8 | Enviar `DEMO` | `DEMO_INICIADA` y el mismo movimiento del servo |
| A9 | Enviar `HOLA` (comando inválido) | `ERROR_COMANDO` |
| A10 | Enviar `ping` en minúsculas | `PONG` (no distingue mayúsculas) |

## B. Pulsadores

| # | Acción | Resultado esperado en el monitor serie |
|---|---|---|
| B1 | Pulsar botón 1 (GPIO32) | `BOTON_1` (una vez por pulsación) |
| B2 | Pulsar botón 2 (GPIO33) | `BOTON_2` |
| B3 | Pulsar botón 3 (GPIO21) | `BOTON_3` |
| B4 | Pulsar botón 4 (GPIO22) | `BOTON_4` |
| B5 | Mantener un botón pulsado varios segundos | Sigue siendo **un solo** mensaje (no repite) |

Sin Python, los botones solo emiten el texto: el LED y el servo no cambian (comportamiento original).

## C. Sensor ultrasónico

Rango de detección: **15–80 cm**. El sensor debe estar en modo ESPERA (LED azul).

| # | Acción | Resultado esperado |
|---|---|---|
| C1 | Wokwi: distancia 200 cm (valor inicial). Físico: nada delante | No aparece `PERSONA_DETECTADA` |
| C2 | Distancia a 50 cm, o una mano a ~50 cm | `PERSONA_DETECTADA` en menos de 1 s |
| C3 | Mantenerla ahí | No se repite |
| C4 | Alejar a >100 cm, esperar ≥9 s, acercar de nuevo | `PERSONA_DETECTADA` otra vez |
| C5 | Acercar de nuevo antes de 9 s | No se repite (enfriamiento) |
| C6 | Enviar `PROCESANDO` y acercar a 50 cm | No se detecta (solo se detecta en ESPERA) |
| C7 | Distancia a 10 cm o a 90 cm | No se detecta (fuera de rango) |

**Si en Wokwi C2 nunca funciona:** puede ser el divisor del ECHO en el simulador. Mira el apartado *Riesgo conocido* del README.

## D. Prueba completa con Python (solo placa real; `utecxina_pc.py` abre puertos COM, no el serie virtual de Wokwi)

Comando: `py pc/utecxina_pc.py --port COM5`. Debe aparecer "Conectado al ESP32 en COM5" y, tras ~2 s, `ESP32 → PC: UTECXINA_LISTA`.

| # | Acción | Resultado esperado |
|---|---|---|
| D1 | Acercar una mano a 15–80 cm | Log `ESP32 → PC: PERSONA_DETECTADA`; la PC dice el saludo; LED **verde** + servo se mueve; al terminar la voz, LED vuelve a **azul** |
| D2 | Botón 1 | Igual que D1 (iniciar interacción) |
| D3 | Botón 3 | Repite la última respuesta (LED verde + servo + voz) |
| D4 | Botón 4 | Demostración: servo + saludo |
| D5 | Botón 2 | LED vuelve a **azul**; la voz que ya sonaba no se corta (limitación original) |
| D6 | Escribir "biblioteca" en la ventana y Enter | LED amarillo → verde con servo → azul; respuesta "Torre 2, piso 5" |
| D7 | Botón *🎤 Hablar* | LED cian mientras escucha → amarillo mientras procesa → verde al responder (requiere micrófono e Internet) |

## E. Pruebas automáticas (en PC, sin hardware)

```
python3 tests/validate_diagram.py        # netlist del circuito: 43 comprobaciones
cd tests && g++ -std=c++17 -I host_mock -DSKETCH='"../../wokwi/sketch.ino"' host_mock/test_firmware.cpp -o fwtest && ./fwtest   # lógica del firmware: 39 comprobaciones
```

Resultado actual: ambas pasan sin fallos. No sustituyen la compilación con el core real de ESP32.

## F. Si algo falla

| Síntoma | Causa probable |
|---|---|
| El LED se ve "al revés" | LED de ánodo común, o `"common":"cathode"` no aplicado en Wokwi |
| Nada responde a los comandos escritos | Monitor serie sin salto de línea; o Python/otro programa tiene el puerto abierto |
| El ESP32 se reinicia al mover el servo | Servo sin fuente propia o sin tierra común; falta el condensador |
| `PERSONA_DETECTADA` aparece sin nadie delante | Falta GND común con el sensor, o ECHO mal conectado |
| Nunca detecta | Divisor mal armado, TRIG/ECHO invertidos, o (Wokwi) ver *Riesgo conocido* |
| Python: "Error serial: could not open port" | Puerto equivocado, o Monitor Serie de Arduino abierto |
