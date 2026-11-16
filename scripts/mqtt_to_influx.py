import json
import logging
import os
import signal
import sys
from datetime import datetime, timezone
from pathlib import Path

import paho.mqtt.client as mqtt
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
)


def leer_secreto(ruta: str) -> str:
    archivo = Path(ruta)
    if not archivo.is_file():
        logging.critical("Archivo de secreto no encontrado en: %s", ruta)
        sys.exit(1)
    contenido = archivo.read_text(encoding="utf-8").strip()
    if not contenido:
        logging.critical("El archivo de secreto esta vacio: %s", ruta)
        sys.exit(1)
    return contenido


def cargar_configuracion():
    try:
        mqtt_cfg = {
            "host": os.environ["MQTT_BROKER_HOST"],
            "port": int(os.environ["MQTT_BROKER_PORT"]),
            "topic": os.environ["MQTT_TOPIC_FILTER"],
        }
        influx_url = os.environ["INFLUXDB_URL"]
        influx_org = os.environ["INFLUXDB_ORG"]
        influx_bucket = os.environ["INFLUXDB_BUCKET"]
        ruta_token = os.environ["INFLUX_TOKEN_FILE"]
    except KeyError as err:
        logging.critical("Variable de configuracion requerida ausente: %s", err)
        sys.exit(1)
    except ValueError as err:
        logging.critical("Formato de puerto MQTT invalido: %s", err)
        sys.exit(1)

    token = leer_secreto(ruta_token)
    cliente_influx = InfluxDBClient(url=influx_url, token=token, org=influx_org)
    del token

    return mqtt_cfg, cliente_influx, influx_org, influx_bucket


def construir_punto(topic: str, payload: dict) -> Point | None:
    partes = topic.split("/")
    if len(partes) < 3:
        return None

    ubicacion, medicion = partes[1], partes[2]
    punto = (
        Point(medicion)
        .tag("location", ubicacion)
        .time(datetime.now(timezone.utc), WritePrecision.NS)
    )

    campos_validos = 0
    for clave, valor in payload.items():
        if isinstance(valor, (int, float)) and not isinstance(valor, bool):
            punto = punto.field(clave, float(valor))
            campos_validos += 1

    return punto if campos_validos > 0 else None


def main():
    mqtt_cfg, influx_client, influx_org, influx_bucket = cargar_configuracion()
    write_api = influx_client.write_api(write_options=SYNCHRONOUS)

    def on_connect(client, userdata, flags, reason_code, properties):
        if reason_code == 0:
            logging.info("Conectado al broker MQTT. Suscribiendo al filtro configurado.")
            client.subscribe(mqtt_cfg["topic"])
        else:
            logging.error("Fallo de conexion MQTT, codigo: %s", reason_code)

    def on_message(client, userdata, msg):
        raw = msg.payload.decode("utf-8", errors="replace")
        try:
            data = json.loads(raw)
            if not isinstance(data, dict):
                logging.warning("Payload descartado (no es objeto JSON) en %s", msg.topic)
                return

            punto = construir_punto(msg.topic, data)
            if punto is None:
                logging.warning("Sin campos numericos validos en %s", msg.topic)
                return

            write_api.write(bucket=influx_bucket, org=influx_org, record=punto)
            logging.info("Punto almacenado desde topico: %s", msg.topic)
        except json.JSONDecodeError:
            logging.error("JSON malformado en topico %s", msg.topic)
        except Exception as err:
            logging.error("Error al escribir en InfluxDB desde %s: %s", msg.topic, err)

    mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="python-influx-bridge")
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message

    def detener_servicio(signum, frame):
        logging.info("Cerrando conexiones de forma limpia...")
        mqtt_client.disconnect()
        influx_client.close()
        sys.exit(0)

    signal.signal(signal.SIGINT, detener_servicio)
    signal.signal(signal.SIGTERM, detener_servicio)

    mqtt_client.connect(mqtt_cfg["host"], mqtt_cfg["port"], keepalive=60)
    mqtt_client.loop_forever()


if __name__ == "__main__":
    main()
