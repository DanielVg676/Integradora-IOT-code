#include <DHT.h>

const int humedadSueloPins[8] = {A0, A1, A2, A3, A4, A5, A6, A7};
const char* humedadSueloIDs[8] = {
  "id_shs_0", "id_shs_1", "id_shs_2", "id_shs_3",
  "id_shs_4", "id_shs_5", "id_shs_6", "id_shs_7"
};

const int ldrPins[4] = {A9, A10, A11, A12};
const char* ldrIDs[4] = {"id_ldr_0", "id_ldr_1", "id_ldr_2", "id_ldr_3"};

const int dhtPins[4] = {50, 49, 48, 47};
const char* dhtTempIDs[4] = {"id_dht_0_temp", "id_dht_1_temp", "id_dht_2_temp", "id_dht_3_temp"};
const char* dhtHumIDs[4]  = {"id_dht_0_hum",  "id_dht_1_hum",  "id_dht_2_hum",  "id_dht_3_hum"};
DHT dhts[4] = {
  DHT(50, DHT11), DHT(49, DHT11), DHT(48, DHT11), DHT(47, DHT11)
};

#define BOMBA1 30
#define BOMBA2 31
#define BOMBA3 32
#define BOMBA4 33

unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 3000;

void setup() {
  Serial.begin(1000000);
  for (int i = 0; i < 4; i++) dhts[i].begin();

  pinMode(BOMBA1, OUTPUT); digitalWrite(BOMBA1, LOW);
  pinMode(BOMBA2, OUTPUT); digitalWrite(BOMBA2, LOW);
  pinMode(BOMBA3, OUTPUT); digitalWrite(BOMBA3, LOW);
  pinMode(BOMBA4, OUTPUT); digitalWrite(BOMBA4, LOW);

  // Mensaje de inicialización en formato JSON válido
  Serial.println("{\"status\":\"init_ok\"}");
}

void loop() {
  unsigned long now = millis();

  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = now;
    sendAllReadings(false);
  }

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    String respuesta;

    if (cmd == "get_all_now") {
      sendAllReadings(true);
    } else if (cmd == "get_realtime_now") {
      sendAllReadings(false);
    } else if (cmd == "bomba1_on") {
      digitalWrite(BOMBA1, HIGH);
      respuesta = "{\"accion\":\"bomba1_on\",\"status\":\"ok\"}";
    } else if (cmd == "bomba1_off") {
      digitalWrite(BOMBA1, LOW);
      respuesta = "{\"accion\":\"bomba1_off\",\"status\":\"ok\"}";
    } else if (cmd == "bomba2_on") {
      digitalWrite(BOMBA2, HIGH);
      respuesta = "{\"accion\":\"bomba2_on\",\"status\":\"ok\"}";
    } else if (cmd == "bomba2_off") {
      digitalWrite(BOMBA2, LOW);
      respuesta = "{\"accion\":\"bomba2_off\",\"status\":\"ok\"}";
    } else if (cmd == "bomba3_on") {
      digitalWrite(BOMBA3, HIGH);
      respuesta = "{\"accion\":\"bomba3_on\",\"status\":\"ok\"}";
    } else if (cmd == "bomba3_off") {
      digitalWrite(BOMBA3, LOW);
      respuesta = "{\"accion\":\"bomba3_off\",\"status\":\"ok\"}";
    } else if (cmd == "bomba4_on") {
      digitalWrite(BOMBA4, HIGH);
      respuesta = "{\"accion\":\"bomba4_on\",\"status\":\"ok\"}";
    } else if (cmd == "bomba4_off") {
      digitalWrite(BOMBA4, LOW);
      respuesta = "{\"accion\":\"bomba4_off\",\"status\":\"ok\"}";
    } else if (cmd == "ping") {
      respuesta = "{\"accion\":\"ping\",\"status\":\"ok\",\"mensaje\":\"pIng\"}";
    } else {
      respuesta = "{\"accion\":\"" + cmd + "\",\"status\":\"error\",\"mensaje\":\"comando desconocido\"}";
    }


    if (respuesta.length() > 0) {
      Serial.println(respuesta);
    }
  }
}

void sendAllReadings(bool isAverage) {
  String tipoStr = isAverage ? ",\"tipo\":\"average\"" : "";
  unsigned long lote = millis();

  Serial.println("{\"inicio\":\"lecturas\"}");
  delay(5);

  // Humedad suelo
  for (int i = 0; i < 8; i++) {
    int valor = analogRead(humedadSueloPins[i]);
    String lectura = "{\"sensorId\":\"" + String(humedadSueloIDs[i]) +
                     "\",\"valor\":" + String(valor) +
                     ",\"sensor\":\"shs\",\"pin\":\"a" + String(i) +
                     "\",\"lote\":" + String(lote) +
                     tipoStr + "}";
    Serial.println(lectura);
    delay(10);
  }

  // DHTs (temp y hum)
  for (int i = 0; i < 4; i++) {
    float temp = dhts[i].readTemperature();
    float hum = dhts[i].readHumidity();

    String tempStr = "{\"sensorId\":\"" + String(dhtTempIDs[i]) +
                     "\",\"valor\":" + (isnan(temp) ? "-1" : String(temp, 1)) +
                     ",\"sensor\":\"temp\",\"pin\":\"d" + String(dhtPins[i]) +
                     "\",\"lote\":" + String(lote) + tipoStr + "}";
    Serial.println(tempStr);

    String humStr = "{\"sensorId\":\"" + String(dhtHumIDs[i]) +
                    "\",\"valor\":" + (isnan(hum) ? "-1" : String(hum, 1)) +
                    ",\"sensor\":\"hum\",\"pin\":\"d" + String(dhtPins[i]) +
                    "\",\"lote\":" + String(lote) + tipoStr + "}";
    Serial.println(humStr);

    delay(10);
  }

  // LDR
  for (int i = 0; i < 4; i++) {
    int valor = analogRead(ldrPins[i]);
    String lectura = "{\"sensorId\":\"" + String(ldrIDs[i]) +
                     "\",\"valor\":" + String(valor) +
                     ",\"sensor\":\"ldr\",\"pin\":\"a" + String(9 + i) +
                     "\",\"lote\":" + String(lote) + tipoStr + "}";
    Serial.println(lectura);
    delay(10);
  }

  Serial.println("{\"fin\":\"lecturas\"}");
  Serial.flush();  // Asegura que todo se envíe antes de que finalice
}

