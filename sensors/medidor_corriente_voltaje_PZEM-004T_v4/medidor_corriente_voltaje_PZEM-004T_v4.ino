#include <HardwareSerial.h>
#include <PZEM004Tv30.h>

HardwareSerial pzemSerial(1);  // UART1 

PZEM004Tv30 pzem(pzemSerial, D9, D10);  // RX=D9(GPIO8), TX=D10(GPIO9)

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Iniciando lectura del PZEM...");
}

void loop() {
  float voltage = pzem.voltage();
  float current = pzem.current();
  float power = pzem.power();
  float energy = pzem.energy();
  float frequency = pzem.frequency();
  float pf = pzem.pf();

  if (isnan(voltage)) {
    Serial.println("Error leyendo el PZEM — revisar cableado/lado AC/dirección Modbus");
  } else {
    Serial.printf("V: %.1f V | I: %.3f A | P: %.1f W | E: %.3f kWh | F: %.1f Hz | PF: %.2f\n",
                  voltage, current, power, energy / 1000.0, frequency, pf);
  }

  delay(2000);
}