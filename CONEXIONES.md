# Conexiones físicas — UTECxina (ESP32-WROOM DevKit)

Todas las conexiones coinciden con `wokwi/diagram.json` (validado con `tests/validate_diagram.py`).

## 1. Conexiones por componente

| # | Componente | Pin del componente | Va a | Notas |
|---|---|---|---|---|
| 1 | HC-SR04 | VCC | +5 V | 5 V regulados (externos, o pin 5V/VIN del DevKit alimentado por USB) |
| 2 | HC-SR04 | TRIG | **GPIO18** | directo; el ESP32 entrega 3,3 V, suficiente para disparar el HC-SR04 |
| 3 | HC-SR04 | ECHO | resistencia **1 kΩ** → nodo ECHO | ECHO nunca directo al ESP32 |
| 4 | Nodo ECHO | (extremo de 1 kΩ) | **GPIO19** | |
| 5 | Resistencia **2 kΩ** | un extremo | **GPIO19** | |
| 6 | Resistencia 2 kΩ | otro extremo | GND | 5 V × 2/3 = **3,33 V** en GPIO19 |
| 7 | HC-SR04 | GND | GND común | |
| 8 | LED RGB (cátodo común) | pata más larga (COM) | GND | **cátodo común**; ver polaridad abajo |
| 9 | LED RGB | R | 220 Ω → **GPIO25** | |
| 10 | LED RGB | G | 220 Ω → **GPIO26** | |
| 11 | LED RGB | B | 220 Ω → **GPIO27** | |
| 12 | Servo SG90 | señal (naranja/amarillo) | **GPIO23** | |
| 13 | Servo SG90 | V+ (rojo) | +5 V **externo** | nunca desde un GPIO ni 9 V |
| 14 | Servo SG90 | GND (marrón/negro) | GND común | tierra compartida con el ESP32 |
| 15 | Pulsador 1 (Iniciar) | terminal A / terminal B | **GPIO32** / GND | `INPUT_PULLUP` interno |
| 16 | Pulsador 2 (Cancelar) | terminal A / terminal B | **GPIO33** / GND | |
| 17 | Pulsador 3 (Repetir) | terminal A / terminal B | **GPIO21** / GND | |
| 18 | Pulsador 4 (Demo) | terminal A / terminal B | **GPIO22** / GND | |

Pulsadores de 4 patas: los terminales A y B deben estar en lados opuestos del botón (las patas del mismo lado están unidas internamente). Compruébalo con el multímetro en continuidad: abierto = sin continuidad, pulsado = continuidad.

## 2. GPIO (resumen)

| Función | GPIO | Función | GPIO |
|---|---|---|---|
| HC-SR04 TRIG | 18 | Servo SG90 | 23 |
| HC-SR04 ECHO (tras divisor) | 19 | Pulsador 1 | 32 |
| LED rojo | 25 | Pulsador 2 | 33 |
| LED verde | 26 | Pulsador 3 | 21 |
| LED azul | 27 | Pulsador 4 | 22 |

Ninguno es pin de flash (6–11), de *strapping* (0, 2, 5, 12, 15) ni de solo entrada (34–39). Cada GPIO se usa una sola vez.

## 3. Alimentación

```
 USB (datos) ──► ESP32 DevKit  (3V3 interno)
 Fuente 5 V regulada ─┬─► servo V+            (obligatorio)
                      └─► HC-SR04 VCC         (o desde el pin 5V/VIN del DevKit)
 GND de la fuente ─── GND del ESP32 ─── GND de todo   (tierra común OBLIGATORIA)
```

- El SG90 puede consumir ~0,5–0,7 A al arrancar o forzar: fuente de 5 V ≥ 1 A.
- **No conectes los 5 V externos al pin 5V/VIN del ESP32 mientras esté por USB** (dos fuentes unidas). Si alimentas servo y sensor externamente, conecta solo el GND.
- **Recomendado (no estaba en tu lista):** condensador electrolítico 470–1000 µF / ≥10 V entre +5 V y GND cerca del servo (respeta la polaridad). Evita reinicios del ESP32 por caídas de tensión cuando se mueve el servo.
- No alimentes el servo con pila de 9 V ni desde el pin 3V3.

## 4. LED RGB: polaridad

El firmware hace `digitalWrite(pin, HIGH)` para encender cada color, es decir, **cátodo común**: la pata común (la más larga) va a **GND**.
Si tu LED es de ánodo común, se verá invertido (todo encendido en reposo): necesitaría cambiar la conexión (común a 3V3) *y* el firmware.
Identifica el tipo con una pila de botón de 3 V + 220 Ω, o con el multímetro en prueba de diodos, antes de montar.

## 5. Lista de materiales

| Cant. | Componente |
|---|---|
| 1 | ESP32-WROOM DevKit (30 o 38 pines) |
| 1 | Sensor HC-SR04 |
| 1 | Servo SG90 |
| 1 | LED RGB 5 mm, cátodo común, 4 patas |
| 4 | Pulsadores 6×6 mm |
| 3 | Resistencias 220 Ω |
| 1 | Resistencia 1 kΩ |
| 1 | Resistencia 2 kΩ |
| 1 | Protoboard de 830 puntos (el DevKit ocupa mucho ancho; una de 400 puede no alcanzar) |
| — | Cables dupont M-M y M-F |
| 1 | Fuente regulada de 5 V (≥1 A) o módulo de alimentación de protoboard con selector 5 V |
| 1 | Cable USB **de datos** |
| 1 | (Recomendado) condensador 470–1000 µF / 10 V+ |

## 6. Sugerencia de protoboard

- Riel superior **+**: 5 V externos. Riel superior **−**: GND común, unido al pin GND del ESP32.
- Servo, HC-SR04 (VCC) y condensador al riel de 5 V; sus GND al riel −.
- Divisor ECHO en el área central: HC-SR04 ECHO → 1 kΩ → fila X; en la fila X: cable a GPIO19 y la 2 kΩ hacia el riel −.
- Pulsadores a caballo del canal central: una pata a la fila con cable a su GPIO, la opuesta al riel −.
- LED RGB: cada color por su 220 Ω hacia su GPIO; pata común al riel −.

## 7. Antes de energizar (multímetro)

1. Continuidad entre +5 V y GND: **debe NO haber** (si pita, hay un corto).
2. Continuidad entre GPIO19 y GND: ≈2 kΩ (por la resistencia). Entre ECHO del sensor y GPIO19: ≈1 kΩ.
3. GND de la fuente, del ESP32, del servo y del sensor: continuidad entre todos.
4. Con todo conectado y los 5 V encendidos, mide GPIO19 contra GND con el sensor midiendo: no debe superar ≈3,3 V.
5. Enciende primero con el servo desconectado y comprueba LED y sensor; después conecta el servo.
