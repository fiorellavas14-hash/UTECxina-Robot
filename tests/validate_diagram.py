#!/usr/bin/env python3
"""Valida wokwi/diagram.json: pines existentes, netlist, divisor, polaridad, alimentacion.
Los nombres de pin de cada pieza Wokwi estan escritos de memoria (no se pudo consultar
Wokwi sin red): si Wokwi rechaza alguno, el error aparecera al abrir el proyecto."""
import json, sys, collections
path = sys.argv[1] if len(sys.argv) > 1 else "wokwi/diagram.json"
d = json.load(open(path))
PINS = {
 "board-esp32-devkit-c-v4": {"3V3","EN","VP","VN","34","35","32","33","25","26","27","14","12","13","D2","D3","CMD","5V",
    "GND.1","GND.2","GND.3","23","22","TX","RX","21","19","18","5","17","16","4","0","2","15","D1","D0","CLK"},
 "wokwi-hc-sr04": {"VCC","TRIG","ECHO","GND"},
 "wokwi-servo": {"PWM","V+","GND"},
 "wokwi-rgb-led": {"R","G","B","COM"},
 "wokwi-resistor": {"1","2"},
 "wokwi-pushbutton": {"1.l","1.r","2.l","2.r"},
}
parts = {p["id"]: p for p in d["parts"]}
err, ok = [], []
def chk(c, msg):
    (ok if c else err).append(msg)

ids = [p["id"] for p in d["parts"]]
chk(len(ids) == len(set(ids)), "ids de piezas unicos")
# pines validos
parent = {}
def find(x):
    parent.setdefault(x, x)
    while parent[x] != x:
        parent[x] = parent[parent[x]]; x = parent[x]
    return x
def union(a, b): parent[find(a)] = find(b)
seen = set()
for a, b, *_ in d["connections"]:
    for ep in (a, b):
        if ep.startswith("$serialMonitor"): continue
        pid, pin = ep.split(":", 1)
        if pid not in parts: err.append(f"pieza inexistente: {ep}"); continue
        if pin not in PINS[parts[pid]["type"]]: err.append(f"pin invalido: {ep}")
    key = frozenset((a, b))
    if key in seen: err.append(f"conexion duplicada: {a} <-> {b}")
    seen.add(key)
    if a == b: err.append(f"conexion consigo misma: {a}")
    union(a, b)
chk(not any("invalido" in e or "inexistente" in e for e in err), "todos los pines/piezas existen")
chk(not any("duplicada" in e for e in err), "sin conexiones duplicadas")
# Pines internamente unidos del pulsador (1.l=1.r, 2.l=2.r)
for pid, p in parts.items():
    if p["type"] == "wokwi-pushbutton":
        union(f"{pid}:1.l", f"{pid}:1.r"); union(f"{pid}:2.l", f"{pid}:2.r")
def net(ep): return find(ep)
def members(ep):
    r = find(ep); return {k for k in parent if find(k) == r}

GND = members("esp:GND.1") | members("esp:GND.2") | members("esp:GND.3")
# Todas las GND del ESP32 son una sola tierra? (en la placa lo estan)
for g in ("esp:GND.1","esp:GND.2","esp:GND.3"): union(g, "esp:GND.1")
GND = members("esp:GND.1")
V5 = members("esp:5V")
chk(not (GND & V5), "5V y GND no estan cortocircuitados")

GPIOS = {"TRIG":"18","ECHO":"19","R":"25","G":"26","B":"27","SERVO":"23","B1":"32","B2":"33","B3":"21","B4":"22"}
gp_nets = {n: net("esp:"+g) for n, g in GPIOS.items()}
chk(len(set(gp_nets.values())) == len(gp_nets), "los 10 GPIO estan en nets distintas (sin cortos entre GPIO)")
for n, g in GPIOS.items():
    chk(("esp:"+g) in parent, f"GPIO{g} ({n}) conectado")
chk(not any(find("esp:"+g) in (find("esp:GND.1"), find("esp:5V")) for g in GPIOS.values()), "ningun GPIO unido a GND o 5V")
# Ningun GPIO extra usado
used = {ep.split(":")[1] for ep in parent if ep.startswith("esp:")} - {"GND.1","GND.2","GND.3","5V","TX","RX"}
chk(used == set(GPIOS.values()), f"GPIO usados == asignacion obligatoria ({sorted(used, key=int)})")
# HC-SR04
chk(net("hc1:TRIG") == gp_nets["TRIG"], "TRIG -> GPIO18 directo")
chk(net("hc1:VCC") == find("esp:5V") and net("hc1:GND") == find("esp:GND.1"), "HC-SR04 VCC=5V, GND comun")
chk(net("hc1:ECHO") != gp_nets["ECHO"], "ECHO NO va directo a GPIO19")
chk(net("hc1:ECHO") == net("r_echo_1k:1") and net("r_echo_1k:2") == gp_nets["ECHO"], "ECHO -> 1k -> GPIO19")
chk(net("r_echo_2k:1") == gp_nets["ECHO"] and net("r_echo_2k:2") == find("esp:GND.1"), "GPIO19 -> 2k -> GND")
chk(parts["r_echo_1k"]["attrs"]["value"] == "1000" and parts["r_echo_2k"]["attrs"]["value"] == "2000", "valores 1k / 2k")
chk(len(members("hc1:ECHO")) == 2, "nodo ECHO solo contiene ECHO y la 1k")
print(f"   divisor: 5 V x 2k/(1k+2k) = {5*2/3:.2f} V en GPIO19")
# LED RGB
chk(parts["rgb1"]["attrs"].get("common") == "cathode", "LED RGB configurado como catodo comun")
chk(net("rgb1:COM") == find("esp:GND.1"), "COM del LED -> GND")
for col, ch in (("red","R"),("green","G"),("blue","B")):
    r = f"r_{col}"
    chk(net(f"{r}:1") == gp_nets[ch] and net(f"{r}:2") == net(f"rgb1:{ch}") and gp_nets[ch] != net(f"rgb1:{ch}"),
        f"GPIO{GPIOS[ch]} -> 220 ohm -> LED {col} (nunca directo)")
    chk(parts[r]["attrs"]["value"] == "220", f"{r} = 220 ohm")
# Servo
chk(net("servo1:PWM") == gp_nets["SERVO"], "servo PWM -> GPIO23")
chk(net("servo1:V+") == find("esp:5V") and not (members("servo1:V+") & {f"esp:{g}" for g in GPIOS.values()}), "servo V+ en 5V, no en un GPIO")
chk(net("servo1:GND") == find("esp:GND.1"), "servo GND comun con ESP32")
# Pulsadores
for i, g in ((1,"32"),(2,"33"),(3,"21"),(4,"22")):
    b = f"btn{i}"
    chk(net(f"{b}:1.l") == net("esp:"+g) and net(f"{b}:2.l") == find("esp:GND.1"), f"pulsador {i}: GPIO{g} <-> GND (sin resistencia externa; pull-up interno)")
    chk(len(members(f"{b}:1.l")) <= 3, f"pulsador {i}: sin otros elementos en su net")
for e in ok: print("OK  ", e)
for e in err: print("FAIL", e)
print(f"\n{len(ok)} comprobaciones OK, {len(err)} fallos")
sys.exit(1 if err else 0)
