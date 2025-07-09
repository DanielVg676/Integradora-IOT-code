import json
import threading
import time
import pika
import serial
import glob
import queue

# ==== DETECT SERIAL PORT ====
def detectar_puerto():
    posibles_puertos = glob.glob('/dev/ttyACM*') + glob.glob('/dev/ttyUSB*')
    if posibles_puertos:
        print(f"✅ Detected port: {posibles_puertos[0]}")
        return posibles_puertos[0]
    else:
        print("❌ Arduino not found.")
        exit()

# ==== CONFIG ====
PUERTO_SERIAL = detectar_puerto()
BAUDIOS = 9600

URL_AMQP = "amqps://oxwjrchs:RHg5HbbZUYwbFadZzjkQ2l3AYcVPCda7@gull.rmq.cloudamqp.com/oxwjrchs"
EXCHANGE = "sensor_exchange"
COLA_SENSORES = "cola_sensores"
COLA_SENSORES_AVG = "cola_sensores_average"
COLA_BOMBAS = "cola_bombas"
COLA_RESPUESTAS = "cola_respuestas"

# ==== SERIAL CONNECTION ====
try:
    arduino = serial.Serial(PUERTO_SERIAL, BAUDIOS, timeout=1)
    time.sleep(2)
    print(f"✅ Connected to {PUERTO_SERIAL}")
except Exception as e:
    print(f"❌ Serial connection error: {e}")
    exit()

# ==== RABBITMQ CONNECTIONS ====
try:
    conexion_lectura = pika.BlockingConnection(pika.URLParameters(URL_AMQP))
    canal_lectura = conexion_lectura.channel()
    canal_lectura.queue_declare(queue=COLA_SENSORES, durable=True)
    canal_lectura.queue_declare(queue=COLA_SENSORES_AVG, durable=True)

    conexion_comandos = pika.BlockingConnection(pika.URLParameters(URL_AMQP))
    canal_comandos = conexion_comandos.channel()
    canal_comandos.queue_declare(queue=COLA_BOMBAS, durable=True)

    conexion_respuestas = pika.BlockingConnection(pika.URLParameters(URL_AMQP))
    canal_respuestas = conexion_respuestas.channel()
    canal_respuestas.queue_declare(queue=COLA_RESPUESTAS, durable=True)
except Exception as e:
    print(f"❌ RabbitMQ connection error: {e}")
    exit()

# ==== COLA DE COMANDOS ====
cola_comandos_serial = queue.Queue()

# ==== PUBLISH FUNCTIONS ====
def enviar_lectura_a_rabbit(msg_json):
    try:
        canal_lectura.basic_publish(
            exchange=EXCHANGE,
            routing_key="sensor.lectura",
            body=json.dumps(msg_json),
            properties=pika.BasicProperties(delivery_mode=2)
        )
        print(f"📤 Sent to {COLA_SENSORES}: {msg_json}")
    except Exception as e:
        print(f"❌ Error sending message: {e}")

def enviar_lectura_average(msg_json):
    try:
        canal_lectura.basic_publish(
            exchange=EXCHANGE,
            routing_key="sensor.average",
            body=json.dumps(msg_json),
            properties=pika.BasicProperties(delivery_mode=2)
        )
        print(f"📤 Sent to {COLA_SENSORES_AVG}: {msg_json}")
    except Exception as e:
        print(f"❌ Error sending average message: {e}")

def enviar_respuesta_bomba(msg_json):
    try:
        canal_respuestas.basic_publish(
            exchange=EXCHANGE,
            routing_key="bomba.respuesta",
            body=json.dumps(msg_json),
            properties=pika.BasicProperties(delivery_mode=2)
        )
        print(f"📤 Pump response sent to {COLA_RESPUESTAS}: {msg_json}")
    except Exception as e:
        print(f"❌ Error sending pump response: {e}")

# ==== THREAD: SERIAL COMMAND PROCESSOR ====
def procesador_de_comandos_serial():
    while True:
        comando, tipo_esperado = cola_comandos_serial.get()
        try:
            arduino.reset_input_buffer()
            arduino.write((comando + "\n").encode())
            print(f"📲 Sent to Arduino: {comando}")

            respuestas = {}
            start_time = time.time()
            while time.time() - start_time < 6:
                if arduino.in_waiting:
                    linea = arduino.readline().decode(errors='ignore').strip()
                    if linea:
                        try:
                            data = json.loads(linea)
                            if isinstance(data, dict):
                                if "accion" in data:
                                    enviar_respuesta_bomba(data)
                                elif tipo_esperado == "average" and data.get("tipo") == "average":
                                    enviar_lectura_average(data)
                                elif tipo_esperado == "realtime" and data.get("sensorId"):
                                    respuestas[data["sensorId"]] = data
                                    if len(respuestas) >= 20:
                                        break
                        except json.JSONDecodeError:
                            print(f"⚠ Invalid JSON: {linea}")

            if tipo_esperado == "realtime":
                for lectura in respuestas.values():
                    enviar_lectura_a_rabbit(lectura)

        except Exception as e:
            print(f"❌ Error processing command '{comando}': {e}")
        cola_comandos_serial.task_done()

# ==== THREAD: ESCUCHAR COMANDOS BOMBA ====
def escuchar_comandos():
    def callback(ch, method, properties, body):
        comando = body.decode().strip()
        print(f"📥 Command received: {comando}")
        cola_comandos_serial.put((comando, "pump"))

    try:
        canal_comandos.basic_consume(queue=COLA_BOMBAS, on_message_callback=callback, auto_ack=True)
        print("🎧 Listening to bomba commands...")
        canal_comandos.start_consuming()
    except Exception as e:
        print(f"❌ Error consuming bomba commands: {e}")

# ==== THREAD: SOLICITAR CADA 30 MIN ====
def leer_y_enviar_30min():
    while True:
        print("⏳ Scheduled 30-min request")
        cola_comandos_serial.put(("get_all_now", "average"))
        time.sleep(1800)

# ==== THREAD: SOLICITAR CADA 15 SEG ====
def leer_y_enviar_realtime():
    while True:
        print("⏱ Scheduled 15-sec realtime request")
        cola_comandos_serial.put(("get_realtime_now", "realtime"))
        time.sleep(15)

# ==== START THREADS ====
th1 = threading.Thread(target=procesador_de_comandos_serial)
th2 = threading.Thread(target=escuchar_comandos)
th3 = threading.Thread(target=leer_y_enviar_30min)
th4 = threading.Thread(target=leer_y_enviar_realtime)

th1.start()
th2.start()
th3.start()
th4.start()
