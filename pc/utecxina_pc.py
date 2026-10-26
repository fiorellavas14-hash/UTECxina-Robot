"""UTECxina V1: ESP32 serial + UI + voz laptop + respuestas locales.
Uso: python utecxina_pc.py --port COM5
Sin hardware: python utecxina_pc.py --demo
"""
import argparse
import queue
import threading
import time
import tkinter as tk
from tkinter import scrolledtext
import unicodedata

parser=argparse.ArgumentParser()
parser.add_argument('--port',default='',help='Puerto COM (ej. COM5)')
parser.add_argument('--demo',action='store_true',help='Interfaz sin ESP32')
args=parser.parse_args()

messages=queue.Queue()
stop=threading.Event()
serial_port=None
try:
    import serial
except ImportError:
    serial=None
try:
    import pyttsx3
except ImportError:
    pyttsx3=None
try:
    import speech_recognition as sr
except ImportError:
    sr=None

def norm(s):
    return ''.join(c for c in unicodedata.normalize('NFD',s.lower()) if unicodedata.category(c)!='Mn')

def answer(q):
    q=norm(q)
    if 'biblioteca' in q:return 'La biblioteca de UTEC se encuentra en la Torre 2, piso 5.'
    if 'cafe' in q or 'comer' in q:return 'El Café está en la Torre 1, piso 6.'
    if 'aula magna' in q:return 'El Aula Magna se encuentra en la Torre 2.'
    if 'sum' in q or 'sala de usos multiples' in q:return 'El SUM está en la Torre 1, piso 2.'
    if 'hola' in q:return '¡Hola! Soy UTECxina. Puedes preguntarme por la biblioteca, El Café, el Aula Magna o el SUM.'
    return 'Todavía no tengo esa información verificada. Puedes preguntarme por la biblioteca, El Café, el Aula Magna o el SUM.'

def emit(kind, value=''):messages.put((kind,value))

def serial_worker():
    global serial_port
    if args.demo or not args.port:
        emit('status','PRIMERA PRUEBA DE UTECXINA XD');return
    if serial is None:
        emit('status','Instala pyserial: pip install pyserial');return
    try:
        serial_port=serial.Serial(args.port,115200,timeout=.2)
        time.sleep(2)
        emit('status','Conectado al ESP32 en '+args.port)
        while not stop.is_set():
            line=serial_port.readline().decode('utf-8',errors='replace').strip()
            if line:emit('event',line)
    except Exception as exc:emit('status','Error serial: '+str(exc))
    finally:
        if serial_port:serial_port.close();serial_port=None

def send(cmd):
    if serial_port and serial_port.is_open:
        try:serial_port.write((cmd+'\n').encode())
        except Exception as exc:emit('status','Error enviando: '+str(exc))
    emit('log','PC → ESP32: '+cmd)

speech_lock=threading.Lock()


def speak(text):
    import asyncio
    import edge_tts
    import pygame
    import tempfile
    import os

    with speech_lock:
        ruta_audio = None

        try:
            # Crear archivo temporal de audio
            with tempfile.NamedTemporaryFile(
                suffix=".mp3",
                delete=False
            ) as archivo:
                ruta_audio = archivo.name

            # Voz femenina peruana
            async def generar_voz():
                voz = edge_tts.Communicate(
                    text=text,
                    voice="es-PE-CamilaNeural",
                    rate="-5%"
                )
                await voz.save(ruta_audio)

            asyncio.run(generar_voz())

            # Reproducir el audio
            if not pygame.mixer.get_init():
                pygame.mixer.init()

            pygame.mixer.music.load(ruta_audio)
            pygame.mixer.music.play()

            while pygame.mixer.music.get_busy():
                pygame.time.wait(100)

            pygame.mixer.music.unload()

        except Exception as exc:
            emit("status", "Error de voz: " + str(exc))

        finally:
            if ruta_audio and os.path.exists(ruta_audio):
                try:
                    os.remove(ruta_audio)
                except OSError:
                    pass

def say_and_reset(text):
    send('RESPONDIENDO');send('MOVER_SERVO');emit('reply',text)
    def worker():
        speak(text)
        emit('idle','')
    threading.Thread(target=worker,daemon=True).start()

def listen_worker():
    if sr is None:emit('status','Instala SpeechRecognition y PyAudio para usar el micrófono');emit('idle','');return
    recognizer=sr.Recognizer()
    try:
        with sr.Microphone() as source:
            recognizer.adjust_for_ambient_noise(source,duration=.5)
            emit('status','Escuchando durante un máximo de 6 segundos...')
            audio=recognizer.listen(source,timeout=6,phrase_time_limit=8)
        send('PROCESANDO')
        # Reconocimiento de Google: requiere conexión a Internet.
        question=recognizer.recognize_google(audio,language='es-PE')
        emit('question',question)
        emit('respond',question)
    except Exception as exc:
        emit('status','No se pudo reconocer la voz: '+str(exc));emit('idle','')

root=tk.Tk();root.title('UTECxina — Asistente interactivo');root.geometry('750x580')
root.configure(bg='#eaf7ff')
heading=tk.Label(root,text='UTECxina 🤖',font=('Segoe UI',25,'bold'),bg='#eaf7ff',fg='#225579');heading.pack(pady=12)
status=tk.StringVar(value='Iniciando...')
tk.Label(root,textvariable=status,bg='#eaf7ff',fg='#225579',wraplength=700).pack()
log=scrolledtext.ScrolledText(root,height=15,font=('Consolas',10),state='disabled');log.pack(fill='both',expand=True,padx=20,pady=10)
entry=tk.Entry(root,font=('Segoe UI',12));entry.pack(fill='x',padx=20,pady=5)

def logline(text):
    log.configure(state='normal');log.insert('end',text+'\n');log.see('end');log.configure(state='disabled')
def ask(event=None):
    question=entry.get().strip();entry.delete(0,'end')
    if question:emit('question',question);send('PROCESANDO');say_and_reset(answer(question))
def listen():send('ESCUCHANDO');threading.Thread(target=listen_worker,daemon=True).start()
def greet():say_and_reset('¡Hola! Soy UTECxina. ¿En qué puedo ayudarte?')
def demo():send('DEMO');greet()
buttons=tk.Frame(root,bg='#eaf7ff');buttons.pack(pady=7)
for label,cmd in [('Preguntar',ask),('🎤 Hablar',listen),('Saludar',greet),('Demo',demo)]:
    tk.Button(buttons,text=label,command=cmd,padx=12,pady=6).pack(side='left',padx=5)
entry.bind('<Return>',ask)

def poll():
    try:
        for _ in range(40):
            kind,val=messages.get_nowait()
            if kind=='status':status.set(val);logline(val)
            elif kind=='log':logline(val)
            elif kind=='event':
                logline('ESP32 → PC: '+val)
                if val in ('PERSONA_DETECTADA','BOTON_1'):greet()
                elif val=='BOTON_2':send('ESPERA');status.set('Interacción cancelada (la voz en curso no se interrumpe)')
                elif val=='BOTON_3':
                    if last_reply[0]:say_and_reset(last_reply[0])
                elif val=='BOTON_4':demo()
            elif kind=='question':logline('Usuario: '+val)
            elif kind=='respond':say_and_reset(answer(val))
            elif kind=='reply':last_reply[0]=val;logline('UTECxina: '+val)
            elif kind=='idle':send('ESPERA')
    except queue.Empty:pass
    if not stop.is_set():root.after(80,poll)
last_reply=['']
def close():stop.set();root.destroy()
root.protocol('WM_DELETE_WINDOW',close)
threading.Thread(target=serial_worker,daemon=True).start()
root.after(80,poll)
root.mainloop()
