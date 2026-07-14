#include <Wire.h>
#include <Adafruit_SHT4x.h>

// Creamos la instancia del sensor SHT40
Adafruit_SHT4x sht40 = Adafruit_SHT4x();

void setup() {
  // Inicializamos la comunicación serie para ver los datos en la PC
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // Espera a que se abra el monitor serie
  }

  Serial.println("--- Inicializando SHT40 ---");

  // Inicializa I2C usando los pines por defecto (D4 como SDA y D5 como SCL en el XIAO)
  Wire.begin(); 

  // Inicializamos el sensor SHT40 en la dirección I2C por defecto (0x44)
  if (!sht40.begin(&Wire)) {
    Serial.println("¡Error! No se pudo encontrar el sensor SHT40.");
    Serial.println("Verifica que las conexiones SDA, SCL, VCC y GND estén firmes.");
    while (1) {
      delay(100); // Se detiene aquí si no encuentra el sensor
    }
  }

  Serial.println("¡SHT40 detectado con éxito!");

  // Configuración del sensor (opcional, por defecto viene en alta precisión)
  sht40.setPrecision(SHT4X_HIGH_PRECISION); // Lectura de la más alta calidad
  sht40.setHeater(SHT4X_NO_HEATER);         // Calentador interno apagado por ahora
}

void loop() {
  // Creamos los contenedores para almacenar las lecturas
  sensors_event_t humidity, temp;
  
  // Obtenemos los eventos de lectura del sensor
  sht40.getEvent(&humidity, &temp);

  // Mostramos las lecturas en el Monitor Serie
  Serial.print("Temperatura: ");
  Serial.print(temp.temperature, 2); // 2 decimales
  Serial.println(" °C");

  Serial.print("Humedad:     ");
  Serial.print(humidity.relative_humidity, 2);
  Serial.println(" % HR");

  Serial.println("-----------------------------");

  // Esperamos 2 segundos antes de la siguiente lectura
  delay(2000);
}