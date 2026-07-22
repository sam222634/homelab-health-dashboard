#include <WiFi.h>
#include <PubSubClient.h>
#include <HardwareSerial.h>
#include <PZEM004Tv30.h>

// --- Configuración WiFi y MQTT ---
const char* ssid = "wifi";
const char* password = "password";
const char* mqtt_server = "ip_broker";
const int mqtt_port = 1883;
const char* mqtt_topic = "home/server-room/power";

const unsigned long publishInterval = 15000; // 15 segundos

// --- Objetos Globales ---
WiFiClient espClient;
PubSubClient client(espClient);

HardwareSerial pzemSerial(1);
PZEM004Tv30 pzem(pzemSerial, D9, D10); // Pines RX: D9, TX: D10

// Variables para el control de energía seguro
float energyOffset = 0.0;
bool offsetEstablecido = false;
unsigned long lastPublish = 0;

// --- Funciones de Red ---
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
    if (client.connect("xiao-esp32s3-power-meter")) { 
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

  // Inicializar red
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  // Intentar obtener un offset válido durante 5 segundos
  Serial.println("Esperando lectura válida del PZEM para estabilizar offset...");
  unsigned long startWait = millis();
  while (millis() - startWait < 5000) {
    float tempEnergy = pzem.energy();
    if (!isnan(tempEnergy)) {
      energyOffset = tempEnergy;
      offsetEstablecido = true;
      Serial.print("Offset inicial fijado con éxito: ");
      Serial.print(energyOffset);
      Serial.println(" Wh");
      break;
    }
    delay(500);
  }

  if (!offsetEstablecido) {
    Serial.println("ADVERTENCIA: No se obtuvo lectura inicial del PZEM. Se fijará en el primer ciclo del loop.");
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

    float voltage = pzem.voltage();
    float current = pzem.current();
    float power   = pzem.power();
    float energy  = pzem.energy();

    // Si cualquier lectura es NaN, abortamos el ciclo para no publicar datos corruptos
    if (isnan(voltage) || isnan(current) || isnan(power) || isnan(energy)) {
      Serial.println("Lectura del PZEM fallida (NaN), se omite este ciclo");
      return;
    }

    // Si falló en el setup, guardamos el primer valor real que entregue el sensor
    if (!offsetEstablecido) {
      energyOffset = energy;
      offsetEstablecido = true;
      Serial.print("Offset fijado exitosamente en bucle: ");
      Serial.print(energyOffset);
      Serial.println(" Wh");
    }

    // Cálculo seguro
    float energyThisPeriod = energy - energyOffset; 

    char payload[128];
    snprintf(payload, sizeof(payload),
              "{\"voltage\":%.1f,\"current\":%.3f,\"power\":%.1f,\"energy_kwh\":%.3f}",
              voltage, current, power, energyThisPeriod / 1000.0);

    // Publicación del mensaje MQTT
    client.publish(mqtt_topic, payload);
    Serial.print("Publicado: ");
    Serial.println(payload);
  }
}