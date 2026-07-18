#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>  // <-- Para el Portal Cautivo y parámetros
#include <PubSubClient.h>
#include <Wire.h>
#include "Adafruit_SHT4x.h"
#include <Preferences.h>  // <-- Librería nativa para guardar la IP del Broker

// --- Configuración dinámica ---
char mqtt_server[40] = "192.168.1.50";  // Valor por defecto (se actualizará con el portal)
const int mqtt_port = 1883;
const char* mqtt_topic = "home/server-room/climate";
const char* r = "-r"; // El mensaje se publica con Retain

const unsigned long publishInterval = 15000; // 15 segundos

// --- Objetos globales ---
WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
unsigned long lastPublish = 0;
Preferences preferences; // Para persistir la IP

void setup_wifi_manager() {
  WiFiManager wm;

  // 1. Cargar la IP guardada previamente en la memoria Flash
  preferences.begin("mqtt_config", false);
  // Si no hay ninguna guardada, usa la de por defecto ("192.168.1.50")
  String saved_ip = preferences.getString("broker_ip", mqtt_server);
  saved_ip.toCharArray(mqtt_server, 40);

  // 2. Crear el campo de texto personalizado para el Portal Web
  // ID: "mqtt", Etiqueta en pantalla: "Servidor MQTT", Valor inicial, largo máx: 40
  WiFiManagerParameter custom_mqtt_server("mqtt", "Servidor MQTT (IP)", mqtt_server, 40);
  wm.addParameter(&custom_mqtt_server);

  Serial.println("Iniciando WiFiManager...");
  
  // 3. Abrir Portal con contraseña en el Access Point (mínimo 8 caracteres)
  if(!wm.autoConnect("ESP32_Config", "unaContraseñaLarga")) {
      Serial.println("Error al conectar o tiempo de espera agotado. Reiniciando...");
      delay(3000);
      ESP.restart();
  }

  // 4. Si se conecta con éxito, leemos lo que el usuario escribió en la web
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  
  // Guardamos la nueva IP en la memoria para el próximo reinicio
  preferences.putString("broker_ip", String(mqtt_server));
  preferences.end();

  Serial.print("¡WiFi Conectado! Usando Broker MQTT en: ");
  Serial.println(mqtt_server);
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
    Serial.print("Conectando al broker MQTT en ");
    Serial.print(mqtt_server);
    Serial.println("...");
    
    if (client.connect("xiao-esp32s3-server-room")) {
      Serial.println("¡MQTT Conectado!");
    } else {
      Serial.print("Fallo en conexión MQTT, rc=");
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

  // Configuramos WiFi y recuperamos/guardamos la IP del Broker
  setup_wifi_manager();
  
  // Iniciamos el cliente MQTT con la IP obtenida
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

    client.publish(mqtt_topic, payload, r);
    Serial.print("Publicado: ");
    Serial.println(payload);
  }
}