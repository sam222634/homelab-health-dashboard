#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "Adafruit_SHT4x.h"

// --- Configuración ---
const char* ssid = "TU_WIFI";
const char* password = "TU_PASSWORD";
const char* mqtt_server = "192.168.1.50";  // IP de la maquina donde corre el broker MQTT
const int mqtt_port = 1883;
const char* mqtt_topic = "home/server-room/climate";
const char* r = "-r"; //El mensaje se publiqca con Retain

const unsigned long publishInterval = 15000; // 15 segundos

// --- Objetos globales ---
WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
unsigned long lastPublish = 0;

void setup_wifi() {
  Serial.print("Conectando a WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" conectado");
}

void ensureWifiConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi caído, reintentando...");
    WiFi.reconnect();
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      delay(500);
    }
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando al broker MQTT...");
    if (client.connect("xiao-esp32s3-server-room")) {
      Serial.println("conectado");
    } else {
      Serial.print("fallo, rc=");
      Serial.print(client.state());
      Serial.println(" reintentando en 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!sht4.begin()) {
    Serial.println("No se encontró el sensor SHT40. Revisa el cableado.");
    while (1) delay(10);
  }
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  ensureWifiConnected();

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastPublish >= publishInterval) {
    lastPublish = now;

    sensors_event_t humidity, temp;
    if (!sht4.getEvent(&humidity, &temp)) {
      Serial.println("Lectura de sensor fallida, se omite este ciclo");
      return;
    }
    if (isnan(temp.temperature) || isnan(humidity.relative_humidity)) {
      Serial.println("Lectura NaN detectada, se omite este ciclo");
      return;
    }

    char payload[64];
    snprintf(payload, sizeof(payload), "{\"temp\":%.1f,\"hum\":%.1f}",
              temp.temperature, humidity.relative_humidity);

    client.publish(mqtt_topic, payload,r);
    Serial.print("Publicado: ");
    Serial.println(payload);
  }
}