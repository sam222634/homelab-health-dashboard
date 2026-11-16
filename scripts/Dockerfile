FROM python:3.12-alpine

RUN adduser -D -u 10001 bridgeuser

WORKDIR /app

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY mqtt_to_influx.py .

USER bridgeuser

CMD ["python", "-u", "mqtt_to_influx.py"]
