#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <HardwareSerial.h>
#include <PZEM004Tv30.h>
#include <Wire.h>
#include "Adafruit_SHT4x.h"

char mqtt_server[40] = "192.168.1.50";
const int mqtt_port = 1883;
const char* topic_power   = "home/server-room/power";
const char* topic_climate = "home/server-room/climate";
const bool MQTT_RETAIN = true;

const unsigned long publishInterval = 15000;

const char* AP_NAME = "ESP32_Config";
const char* AP_PASS = "unaContraseñaLarga";
const unsigned long portalTimeoutSec = 180;
const int maxMqttFallos = 3;
int mqttFallos = 0;

WiFiClient espClient;
PubSubClient client(espClient);
Preferences preferences;
WiFiManager wm;
WiFiManagerParameter custom_mqtt_server("mqtt", "Servidor MQTT (IP)", mqtt_server, 40);

HardwareSerial pzemSerial(1);
PZEM004Tv30 pzem(pzemSerial, D9, D10);

Adafruit_SHT4x sht4 = Adafruit_SHT4x();
bool sht4Disponible = false;

float energyOffset = 0.0;
bool offsetEstablecido = false;
unsigned long lastPublish = 0;

void guardarIpBroker() {
  strncpy(mqtt_server, custom_mqtt_server.getValue(), sizeof(mqtt_server) - 1);
  mqtt_server[sizeof(mqtt_server) - 1] = '\0';

  preferences.begin("mqtt_config", false);
  preferences.putString("broker_ip", mqtt_server);
  preferences.end();

  client.setServer(mqtt_server, mqtt_port);
  Serial.print("Broker MQTT actualizado: ");
  Serial.println(mqtt_server);
}

void abrirPortal() {
  Serial.println("Abriendo portal de configuracion...");
  client.disconnect();
  custom_mqtt_server.setValue(mqtt_server, 40);
  wm.setConfigPortalTimeout(portalTimeoutSec);

  if (!wm.startConfigPortal(AP_NAME, AP_PASS)) {
    Serial.println("Tiempo de espera del portal agotado. Reiniciando...");
    delay(3000);
    ESP.restart();
  }

  guardarIpBroker();
  mqttFallos = 0;
}

void setup_wifi_manager() {
  preferences.begin("mqtt_config", true);
  preferences.getString("broker_ip", mqtt_server, sizeof(mqtt_server));
  preferences.end();

  custom_mqtt_server.setValue(mqtt_server, 40);
  wm.addParameter(&custom_mqtt_server);
  wm.setConfigPortalTimeout(portalTimeoutSec);

  Serial.println("Iniciando WiFiManager...");

  if (!wm.autoConnect(AP_NAME, AP_PASS)) {
    Serial.println("Fallo de conexion WiFi o tiempo agotado. Reiniciando...");
    delay(3000);
    ESP.restart();
  }

  guardarIpBroker();

  Serial.print("WiFi conectado. Broker MQTT: ");
  Serial.println(mqtt_server);
}

void ensureWifiConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Conexion WiFi perdida. Reintentando...");
    WiFi.reconnect();
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      delay(500);
    }
    if (WiFi.status() != WL_CONNECTED) {
      abrirPortal();
    }
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando al broker MQTT en ");
    Serial.print(mqtt_server);
    Serial.println("...");

    if (client.connect("xiao-esp32s3-server-room")) {
      Serial.println("MQTT conectado.");
      mqttFallos = 0;
    } else {
      mqttFallos++;
      Serial.print("Fallo de conexion MQTT, rc=");
      Serial.print(client.state());
      Serial.print(" (intento ");
      Serial.print(mqttFallos);
      Serial.print("/");
      Serial.print(maxMqttFallos);
      Serial.println(")");

      if (mqttFallos >= maxMqttFallos) {
        abrirPortal();
      } else {
        delay(5000);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!sht4.begin()) {
    Serial.println("Advertencia: No se detecto el sensor SHT40 en el bus I2C.");
    sht4Disponible = false;
  } else {
    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);
    sht4Disponible = true;
  }

  setup_wifi_manager();
  client.setServer(mqtt_server, mqtt_port);

  Serial.println("Esperando lectura del PZEM para fijar offset de energia...");
  unsigned long startWait = millis();
  while (millis() - startWait < 5000) {
    float tempEnergy = pzem.energy();
    if (!isnan(tempEnergy)) {
      energyOffset = tempEnergy;
      offsetEstablecido = true;
      Serial.print("Offset inicial fijado: ");
      Serial.print(energyOffset);
      Serial.println(" Wh");
      break;
    }
    delay(500);
  }

  if (!offsetEstablecido) {
    Serial.println("Advertencia: No se obtuvo lectura inicial del PZEM. Se fijara en el primer ciclo.");
  }
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

    if (sht4Disponible) {
      sensors_event_t humidity, temp;
      if (!sht4.getEvent(&humidity, &temp) ||
          isnan(temp.temperature) || isnan(humidity.relative_humidity)) {
        Serial.println("Lectura SHT40 fallida, se omite metrica de clima.");
      } else {
        char payloadClima[64];
        snprintf(payloadClima, sizeof(payloadClima), "{\"temp\":%.1f,\"hum\":%.1f}",
                 temp.temperature, humidity.relative_humidity);
        client.publish(topic_climate, payloadClima, MQTT_RETAIN);
        Serial.print("Publicado clima: ");
        Serial.println(payloadClima);
      }
    }

    float voltage = pzem.voltage();
    float current = pzem.current();
    float power   = pzem.power();
    float energy  = pzem.energy();

    if (isnan(voltage) || isnan(current) || isnan(power) || isnan(energy)) {
      Serial.println("Lectura PZEM fallida (NaN), se omite metrica de potencia.");
    } else {
      if (!offsetEstablecido) {
        energyOffset = energy;
        offsetEstablecido = true;
        Serial.print("Offset fijado en ciclo: ");
        Serial.print(energyOffset);
        Serial.println(" Wh");
      }

      float energyThisPeriod = energy - energyOffset;

      char payloadPower[128];
      snprintf(payloadPower, sizeof(payloadPower),
               "{\"voltage\":%.1f,\"current\":%.3f,\"power\":%.1f,\"energy_kwh\":%.3f}",
               voltage, current, power, energyThisPeriod / 1000.0);

      client.publish(topic_power, payloadPower, MQTT_RETAIN);
      Serial.print("Publicado potencia: ");
      Serial.println(payloadPower);
    }
  }
}
