#include <DHT.h>

// ==== HUMEDAD DE SUELO ====
const int humedadSueloPins[8] = {A0, A1, A2, A3, A4, A5, A6, A7};
const char* humedadSueloIDs[8] = {
  "id_shs_0", "id_shs_1", "id_shs_2", "id_shs_3",
  "id_shs_4", "id_shs_5", "id_shs_6", "id_shs_7"
};

// ==== FOTOSENSORES LDR ====
const int ldrPins[4] = {A9, A10, A11, A12};
const char* ldrIDs[4] = {"id_ldr_0", "id_ldr_1", "id_ldr_2", "id_ldr_3"};

// ==== DHT11 ====
const int dhtPins[4] = {50, 49, 48, 47};
const char* dhtIDs[4] = {"id_dht_0", "id_dht_1", "id_dht_2", "id_dht_3"};
DHT dhts[4] = {
  DHT(50, DHT11), DHT(49, DHT11), DHT(48, DHT11), DHT(47, DHT11)
};

// ==== RELÉS ====
#define BOMBA1 30
#define BOMBA2 31
#define BOMBA3 32
#define BOMBA4 33

// ==== TIMING ====
// Intervalos de tiempo para liberar carga al arduino mega y asi asegurar las lecturas correctas de los sensores y comandos
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 3000;

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < 4; i++) dhts[i].begin();

  pinMode(BOMBA1, OUTPUT);
  pinMode(BOMBA2, OUTPUT);
  pinMode(BOMBA3, OUTPUT);
  pinMode(BOMBA4, OUTPUT);

  digitalWrite(BOMBA1, LOW);
  digitalWrite(BOMBA2, LOW);
  digitalWrite(BOMBA3, LOW);
  digitalWrite(BOMBA4, LOW);

  Serial.println("Sistema de sensores y 4 bombas iniciado.");
}

void loop() {
  unsigned long now = millis();

  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = now;

    // === HUMEDAD DE SUELO ===
    for (int i = 0; i < 8; i++) {
      int valor = analogRead(humedadSueloPins[i]);
      Serial.print("{\"sensorId\":\"");
      Serial.print(humedadSueloIDs[i]);
      Serial.print("\",\"valor\":");
      Serial.print(valor);
      Serial.print(",\"sensor\":\"shs\",\"pin\":\"a");
      Serial.print(i);
      Serial.println("\"}");
      delay(20);
    }

    // === DHT11 (Temperatura y Humedad) ===
    for (int i = 0; i < 4; i++) {
      float temp = dhts[i].readTemperature();
      float hum = dhts[i].readHumidity();

      Serial.print("{\"sensorId\":\"");
      Serial.print(dhtIDs[i]);
      Serial.print("\",\"valor\":");
      Serial.print(isnan(temp) ? "\"error\"" : String(temp, 1));
      Serial.print(",\"sensor\":\"temp\",\"pin\":\"d");
      Serial.print(dhtPins[i]);
      Serial.println("\"}");

      Serial.print("{\"sensorId\":\"");
      Serial.print(dhtIDs[i]);
      Serial.print("\",\"valor\":");
      Serial.print(isnan(hum) ? "\"error\"" : String(hum, 1));
      Serial.print(",\"sensor\":\"hum\",\"pin\":\"d");
      Serial.print(dhtPins[i]);
      Serial.println("\"}");

      delay(50);
    }

    // === FOTOSENSORES ===
    for (int i = 0; i < 4; i++) {
      int valor = analogRead(ldrPins[i]);
      Serial.print("{\"sensorId\":\"");
      Serial.print(ldrIDs[i]);
      Serial.print("\",\"valor\":");
      Serial.print(valor);
      Serial.print(",\"sensor\":\"ldr\",\"pin\":\"a");
      Serial.print(9 + i);
      Serial.println("\"}");
      delay(20);
    }
  }

  // === ESCUCHAR COMANDOS ===
  // Lectura de comandos via serial monitor para la activacion o desactivacion de las bombas
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    String respuesta;

    if (cmd == "bomba1_on") {
      digitalWrite(BOMBA1, HIGH);
      respuesta = "{\"accion\":\"bomba1_on\",\"status\":\"ok\",\"mensaje\":\"Bomba 1 activada\"}";
    } else if (cmd == "bomba1_off") {
      digitalWrite(BOMBA1, LOW);
      respuesta = "{\"accion\":\"bomba1_off\",\"status\":\"ok\",\"mensaje\":\"Bomba 1 desactivada\"}";

    } else if (cmd == "bomba2_on") {
      digitalWrite(BOMBA2, HIGH);
      respuesta = "{\"accion\":\"bomba2_on\",\"status\":\"ok\",\"mensaje\":\"Bomba 2 activada\"}";
    } else if (cmd == "bomba2_off") {
      digitalWrite(BOMBA2, LOW);
      respuesta = "{\"accion\":\"bomba2_off\",\"status\":\"ok\",\"mensaje\":\"Bomba 2 desactivada\"}";

    } else if (cmd == "bomba3_on") {
      digitalWrite(BOMBA3, HIGH);
      respuesta = "{\"accion\":\"bomba3_on\",\"status\":\"ok\",\"mensaje\":\"Bomba 3 activada\"}";
    } else if (cmd == "bomba3_off") {
      digitalWrite(BOMBA3, LOW);
      respuesta = "{\"accion\":\"bomba3_off\",\"status\":\"ok\",\"mensaje\":\"Bomba 3 desactivada\"}";

    } else if (cmd == "bomba4_on") {
      digitalWrite(BOMBA4, HIGH);
      respuesta = "{\"accion\":\"bomba4_on\",\"status\":\"ok\",\"mensaje\":\"Bomba 4 activada\"}";
    } else if (cmd == "bomba4_off") {
      digitalWrite(BOMBA4, LOW);
      respuesta = "{\"accion\":\"bomba4_off\",\"status\":\"ok\",\"mensaje\":\"Bomba 4 desactivada\"}";

    } else {
      respuesta = "{\"accion\":\"" + cmd + "\",\"status\":\"error\",\"mensaje\":\"Comando desconocido\"}";
    }

    Serial.println(respuesta);
  }
}
