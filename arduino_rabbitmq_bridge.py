import json
import threading
import time
import pika
import serial
import glob

# ==== FUNCIÓN PARA DETECTAR EL PUERTO ====
def detectar_puerto():
    posibles_puertos = glob.glob('/dev/ttyACM*') + glob.glob('/dev/ttyUSB*')
    if posibles_puertos:
        print(f"✅ Puerto detectado: {posibles_puertos[0]}")
        return posibles_puertos[0]
    else:
        print("❌ No se encontró el Arduino.")
        exit()

# ==== CONFIGURACIÓN ====
PUERTO_SERIAL = detectar_puerto()
BAUDIOS = 9600

URL_AMQP = "amqps://oxwjrchs:RHg5HbbZUYwbFadZzjkQ2l3AYcVPCda7@gull.rmq.cloudamqp.com/oxwjrchs"
EXCHANGE = "sensor_exchange"
COLA_SENSORES = "cola_sensores"
COLA_BOMBAS = "cola_bombas"
COLA_RESPUESTAS = "cola_respuestas"

# ==== CONEXIÓN SERIAL ====
try:
    arduino = serial.Serial(PUERTO_SERIAL, BAUDIOS, timeout=1)
    time.sleep(2)
    print(f"✅ Conectado a {PUERTO_SERIAL}")
except Exception as e:
    print(f"❌ Error al conectar al puerto serial: {e}")
    exit()

# ==== CONEXIONES RABBITMQ ====
try:
    conexion_lectura = pika.BlockingConnection(pika.URLParameters(URL_AMQP))
    canal_lectura = conexion_lectura.channel()
    canal_lectura.queue_declare(queue=COLA_SENSORES, durable=True)

    conexion_comandos = pika.BlockingConnection(pika.URLParameters(URL_AMQP))
    canal_comandos = conexion_comandos.channel()
    canal_comandos.queue_declare(queue=COLA_BOMBAS, durable=True)

    conexion_respuestas = pika.BlockingConnection(pika.URLParameters(URL_AMQP))
    canal_respuestas = conexion_respuestas.channel()
    canal_respuestas.queue_declare(queue=COLA_RESPUESTAS, durable=True)
except Exception as e:
    print(f"❌ Error al conectar a RabbitMQ: {e}")
    exit()

# ==== FUNCIONES ====

def enviar_lectura_a_rabbit(msg_json):
    try:
        canal_lectura.basic_publish(
            exchange=EXCHANGE,
            routing_key="sensor.lectura",
            body=json.dumps(msg_json),
            properties=pika.BasicProperties(delivery_mode=2)
        )
        print(f"📤 Enviado a {COLA_SENSORES}: {msg_json}")
    except Exception as e:
        print(f"❌ Error al enviar mensaje: {e}")

def enviar_respuesta_bomba(msg_json):
    try:
        canal_respuestas.basic_publish(
            exchange=EXCHANGE,
            routing_key="bomba.respuesta",
            body=json.dumps(msg_json),
            properties=pika.BasicProperties(delivery_mode=2)
        )
        print(f"📤 Respuesta enviada a {COLA_RESPUESTAS}: {msg_json}")
    except Exception as e:
        print(f"❌ Error al enviar respuesta de bomba: {e}")

def escuchar_serial():
    buffer = []
    ultimo_envio = time.time()

    while True:
        try:
            if arduino.in_waiting:
                linea = arduino.readline().decode(errors='ignore').strip()
                if linea:
                    try:
                        data = json.loads(linea)
                        # Clasifica si es lectura de sensor o respuesta de bomba
                        if isinstance(data, dict) and "accion" in data:
                            enviar_respuesta_bomba(data)
                        else:
                            buffer.append(data)
                    except json.JSONDecodeError:
                        print(f"⚠ Línea ignorada (no es JSON válido): {linea}")

            # Si han pasado 10 segundos, envía el lote de lecturas
            if time.time() - ultimo_envio >= 10:
                for lectura in buffer:
                    enviar_lectura_a_rabbit(lectura)
                buffer.clear()
                ultimo_envio = time.time()

        except Exception as e:
            print(f"❌ Error en lectura serial: {e}")
            break

def escuchar_comandos():
    def callback(ch, method, properties, body):
        comando = body.decode().strip()
        print(f"📥 Comando recibido: {comando}")
        try:
            arduino.write((comando + "\n").encode())
        except Exception as e:
            print(f"❌ Error al enviar comando al Arduino: {e}")

    try:
        canal_comandos.basic_consume(queue=COLA_BOMBAS, on_message_callback=callback, auto_ack=True)
        print("🎧 Escuchando comandos desde cola_bombas...")
        canal_comandos.start_consuming()
    except Exception as e:
        print(f"❌ Error al consumir mensajes: {e}")

# ==== INICIAR HEBRAS ====
h1 = threading.Thread(target=escuchar_serial)
h2 = threading.Thread(target=escuchar_comandos)

h1.start()
h2.start()
