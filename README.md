# homelab-health-dashboard
Un sistema integral de observabilidad IoT diseñado para monitorear la salud ambiental (temperatura) y el consumo energético de un servidor casero 24/7. 

Este proyecto integra hardware de grado industrial con un stack moderno de DevOps para recolectar, procesar y visualizar métricas en tiempo real, operando en una red virtual segmentada (Proxmox).

## Arquitectura del Sistema

El flujo de datos sigue un modelo de publicación/suscripción (Pub/Sub) completamente contenerizado:

1. **Hardware Edge:** Un microcontrolador Seeed Studio XIAO ESP32S3 lee sensores distintos.
2. **Mensajería:** Los datos se publican vía WiFi a un broker **Eclipse Mosquitto (MQTT)**.
3. **Procesamiento:** Un script en **Python** se suscribe a los tópicos, formatea los payloads y los inyecta en la base de datos.
4. **Almacenamiento:** **InfluxDB** guarda las métricas optimizadas como series de tiempo.
5. **Visualización y Alertas:** **Grafana** consume los datos para el dashboard y dispara alertas críticas a través de un bot de **Telegram**.
6. **Acceso Seguro:** Todo el entorno está expuesto de forma segura mediante **Tailscale (VPN Mesh)**, evitando abrir puertos al internet público.

## Hardware y Sensores

Se utilizaron sensores de alta precisión (I2C y UART), superando el enfoque tradicional de sensores analógicos básicos:

| Sensor | Medición | Protocolo | Justificación Técnica |
| :--- | :--- | :--- | :--- |
| **PZEM-004T v4** | Voltaje, Corriente, kWh | UART (Modbus-RTU) | Conversión interna de grado industrial, evitando cálculos manuales imprecisos. |
| **SHT40** | Temperatura y Humedad | I2C | Alta fiabilidad; provee datos de compensación cruzada para el sensor VOC. |


## Decisiones Técnicas y Retos Resueltos

* **InfluxDB vs Bases Relacionales:** Se optó por una base de datos de series de tiempo (TSDB) ya que la naturaleza de los datos IoT no requiere un modelo relacional. Esto optimiza las consultas temporales y permite políticas de retención automáticas (Data Lifecycle).


## Instalación y Despliegue

Toda la infraestructura de software se levanta utilizando Docker Compose.

1. Clona el repositorio:
   ```bash
   git clone [https://github.com/tu-usuario/homelab-health-dashboard.git](https://github.com/tu-usuario/homelab-health-dashboard.git)
   cd homelab-health-dashboard
