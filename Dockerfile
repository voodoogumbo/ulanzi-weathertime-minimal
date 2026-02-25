FROM python:3.11-slim

WORKDIR /app

RUN pip install --no-cache-dir paho-mqtt pytz

COPY mqtt_time_recycling_publisher.py .

CMD ["python3", "-u", "mqtt_time_recycling_publisher.py"]
