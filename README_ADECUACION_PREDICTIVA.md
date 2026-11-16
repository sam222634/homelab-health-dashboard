# Sistema de Monitoreo Termo-Eléctrico y Mantenimiento Predictivo Multi-Escenario (Adecuación Cadena de Frío, Motores y Auditoría)

Plataforma integral de observabilidad IoT de grado industrial diseñada para correlacionar en tiempo real variables higrotérmicas (temperatura y humedad relativa) con el perfil eléctrico completo de cargas en corriente alterna (voltaje, corriente, potencia activa, energía acumulada, frecuencia y factor de potencia).

El proyecto está estructurado en ramas según el escenario de implementación:
* **Rama base (`main` / `feat/Add-esp32/Pruebas-MQTT`):** Enfocada en la salud ambiental y eficiencia energética (PUE) de infraestructura de servidores locales (*Homelab Health Dashboard*).
* **Rama de adecuación industrial/comercial (`feat/monitoreo-predictivo-motores-frio`):** Enfocada en sectores críticos de **Cadena de Frío (Alimentos y Farmacia)**, **Mantenimiento Predictivo de Motores Eléctricos** y **Auditoría Energética Portátil**.

---

## Objetivo General

Desarrollar e implementar un sistema de telemetría y diagnóstico temprano basado en el microcontrolador **Seeed Studio XIAO ESP32S3** y sensores digitales de alta precisión (**PZEM-004T v4** y **SHT40**), capaz de detectar anomalías electromecánicas en motores/compresores y desviaciones térmicas antes de que ocurra la pérdida de insumos críticos (medicamentos, vacunas, alimentos perecederos) o el daño catastrófico de maquinaria, utilizando una infraestructura autohospedada en contenedores Docker (**Mosquitto MQTT, Python, InfluxDB, Grafana y alertas vía Telegram**).

---

## Los 3 Ámbitos de Aplicación (Enfoque Multi-Propósito)

Gracias a que el prototipo mide simultáneamente el comportamiento eléctrico de la máquina y su efecto térmico en el entorno, el mismo hardware cubre tres casos de uso reales con énfasis prioritario en el sector alimentario y farmacéutico:

### 1. Cadena de Frío en Sector Alimentario y Farmacéutico (Énfasis Principal)
En farmacias, clínicas, laboratorios, carnicerías y tiendas de abarrotes, un fallo nocturno en un refrigerador o cámara fría provoca la pérdida total de medicamentos termolábiles (insulina, reactivos, vacunas) o alimentos.
* **Qué aporta el sistema:** A diferencia de un termómetro convencional que alerta cuando la temperatura interna ya subió y el producto está en riesgo, este sistema monitorea el **ciclo de trabajo eléctrico del motor compresor** junto con la **temperatura y humedad**, detectando:
  * **Cortes eléctricos o fallo de relé/termostato:** La temperatura empieza a subir mientras la corriente permanece en `0.000 A`.
  * **Puerta mal cerrada o empaque dañado:** Aumento súbito de humedad relativa + compresor encendido al 100% sin lograr bajar la temperatura.
  * **Fuga de gas refrigerante o condensador obstruido:** Consumo eléctrico ininterrumpido durante horas sin descenso térmico proporcional.

### 2. Mantenimiento Predictivo en Motores Eléctricos y Bombas de Agua (IIoT)
Aplicado a bombas hidráulicas, compresores de aire y extractores en talleres, edificios o sistemas de riego.
* **Qué aporta el sistema:** Evalúa la salud inductiva del motor mediante el **Factor de Potencia (`pf`)**, la **Corriente RMS (`current`)** y la **Temperatura de carcasa (`temp`)**:
  * **Desgaste de rodamientos o sobrecarga mecánica:** Incremento progresivo del amperaje por encima del valor nominal y elevación térmica.
  * **Trabajo en vacío (cavitación en bombas sin agua):** Caída anormal tanto en la corriente como en el factor de potencia (`pf < 0.50`).
  * **Rotor bloqueado (*Locked Rotor*):** Picos repetitivos de corriente de arranque seguidos de cortes a `0 A` por disparo del protector térmico (*klixon*).

### 3. Auditoría Energética Portátil ("Plug & Play") y Cuartos de Servidores
Gracias al portal cautivo dinámico (`WiFiManager`) y almacenamiento en memoria Flash no volátil (`Preferences` NVS), el nodo puede trasladarse a cualquier instalación, conectarse a la red Wi-Fi local desde un teléfono móvil sin reprogramar el microcontrolador y auditar el consumo en `kWh`, factor de potencia y costo tarifario.

---

## Análisis de Mercado y Ventajas Competitivas

En el mercado actual existen tres familias de productos comerciales, pero ninguna resuelve de forma conjunta y accesible el diagnóstico termo-eléctrico para PyMEs:

| Característica | Data Loggers Comerciales (*Elitech / Testo*) | Enchufes Inteligentes (*Sonoff POW / Shelly*) | Sistemas BMS Industriales (*Emerson / Carel*) | **Este Proyecto (XIAO ESP32S3 + PZEM + SHT40)** |
| :--- | :---: | :---: | :---: | :---: |
| **Precisión térmica digital ($\pm 0.2\,^\circ\text{C}$)** | Sí | No | Sí | **Sí (SHT40 por I2C)** |
| **Variables eléctricas completas (V, I, W, kWh, Hz, PF)** | No | Parcial | Sí | **Sí (6 registros Modbus-RTU)** |
| **Diagnóstico cruzado (Temperatura vs. Motor)** | No | No | Sí | **Sí (Correlación en tiempo real)** |
| **Alerta predictiva (antes de perder la cadena de frío)** | No (Reactivo) | No | Sí | **Sí (Detecta omisión o bloqueo de ciclo)** |
| **Sin pago de suscripciones mensuales a la nube** | No (SaaS anual) | Parcial | No (Licencias) | **Sí (100% Autohospedado en Docker)** |
| **Portal cautivo portátil sin reprogramar código** | Sí | Sí | No | **Sí (`WiFiManager` + `Preferences`)** |
| **Costo aproximado de implementación** | $2,000 – $9,000 MXN | $500 – $1,500 MXN | > $25,000 MXN | **~ $550 – $750 MXN** |

---

## Hardware y Sensores

| Sensor | Medición | Protocolo | Justificación Técnica en la Adecuación |
| :--- | :--- | :--- | :--- |
| **PZEM-004T v4** | Voltaje (V), Corriente (A), Potencia (W), Energía (kWh), Frecuencia (Hz) y Factor de Potencia (PF) | UART1 (Modbus-RTU) | Mide las 6 variables eléctricas del motor/compresor con conversión interna por hardware, permitiendo evaluar el Factor de Potencia y picos de arranque. |
| **Adafruit SHT40** | Temperatura (°C) y Humedad Relativa (%RH) | I2C | Precisión industrial ($\pm 0.2\,^\circ\text{C}$) para vigilar la cámara de conservación (alimentos/fármacos) o la disipación térmica en la carcasa del motor. |

---

## Instalación y Despliegue

Toda la infraestructura de software se levanta utilizando Docker Compose:

```bash
git clone -b feat/monitoreo-predictivo-motores-frio https://github.com/sam222634/homelab-health-dashboard.git
cd homelab-health-dashboard
cp .env.example .env
docker compose up -d
```
