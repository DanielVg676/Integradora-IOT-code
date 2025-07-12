import pika

# URL de conexión a CloudAMQP
URL_AMQP = "amqps://oxwjrchs:RHg5HbbZUYwbFadZzjkQ2l3AYcVPCda7@gull.rmq.cloudamqp.com/oxwjrchs"

# Exchange principal
EXCHANGE = "sensor_exchange"

# Diccionario de colas y sus respectivas routing keys
COLAS = {
    "cola_sensores": "sensor.lectura",
    "cola_sensores_average": "sensor.average",  # ✅ NUEVA COLA para almacenamiento cada 30 min
    "cola_bombas": "bomba.control",
    "cola_respuestas": "bomba.respuesta"
}

# Conexión a RabbitMQ
connection = pika.BlockingConnection(pika.URLParameters(URL_AMQP))
channel = connection.channel()

# Crear el exchange (tipo direct)
channel.exchange_declare(exchange=EXCHANGE, exchange_type='direct', durable=True)

# Crear cada cola y vincularla al exchange con su routing key
for cola, routing_key in COLAS.items():
    channel.queue_declare(queue=cola, durable=True)
    channel.queue_bind(exchange=EXCHANGE, queue=cola, routing_key=routing_key)
    print(f"✅ Cola '{cola}' vinculada con routing key '{routing_key}'")

# Cerrar la conexión
connection.close()
print("✅ Configuración completa.")