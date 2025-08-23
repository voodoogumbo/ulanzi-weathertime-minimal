#!/usr/bin/env python3
"""
MQTT Time Publisher for TC001 Enhanced Clock

This script publishes Unix timestamps to your MQTT broker for the TC001 clock
to use as its time source instead of NTP.

Usage:
    python3 mqtt_time_publisher.py

Requirements:
    pip install paho-mqtt

Author: Claude Code Assistant
License: MIT
"""

import time
import json
import paho.mqtt.client as mqtt
import logging
import sys
from datetime import datetime

# Handle paho-mqtt version compatibility (1.x vs 2.x)
try:
    from paho.mqtt.client import CallbackAPIVersion
    MQTT_V2 = True
except ImportError:
    MQTT_V2 = False

# Configuration - Update these to match your config.h settings
MQTT_HOST = "192.168.1.100"    # Your MQTT broker IP address (match config.h)
MQTT_PORT = 1883               # Standard MQTT port
MQTT_BASE = "tc001"            # Base topic for all MQTT messages (match config.h)
MQTT_USERNAME = None           # Set if your broker requires authentication
MQTT_PASSWORD = None           # Set if your broker requires authentication

# Publishing settings
PUBLISH_INTERVAL = 60          # Publish every 60 seconds
CLIENT_ID = "tc001-time-publisher"

# Logging setup
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class TimePublisher:
    def __init__(self):
        # Create MQTT client with version compatibility
        if MQTT_V2:
            # paho-mqtt 2.x requires callback_api_version
            self.client = mqtt.Client(client_id=CLIENT_ID, callback_api_version=CallbackAPIVersion.VERSION1)
        else:
            # paho-mqtt 1.x doesn't have callback_api_version
            self.client = mqtt.Client(CLIENT_ID)
            
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_publish = self.on_publish
        self.connected = False
        
        # Set authentication if configured
        if MQTT_USERNAME and MQTT_PASSWORD:
            self.client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    
    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.connected = True
            logger.info(f"Connected to MQTT broker at {MQTT_HOST}:{MQTT_PORT}")
            logger.info(f"Publishing to topic: {MQTT_BASE}/time")
        else:
            logger.error(f"Failed to connect to MQTT broker. Return code: {rc}")
            self.connected = False
    
    def on_disconnect(self, client, userdata, rc):
        self.connected = False
        logger.warning(f"Disconnected from MQTT broker. Return code: {rc}")
    
    def on_publish(self, client, userdata, mid):
        logger.debug(f"Message published successfully (mid: {mid})")
    
    def connect(self):
        try:
            logger.info(f"Connecting to MQTT broker {MQTT_HOST}:{MQTT_PORT}...")
            self.client.connect(MQTT_HOST, MQTT_PORT, 60)
            self.client.loop_start()
            
            # Wait for connection
            wait_time = 0
            while not self.connected and wait_time < 10:
                time.sleep(0.5)
                wait_time += 0.5
            
            if not self.connected:
                logger.error("Failed to connect within timeout period")
                return False
            
            return True
        except Exception as e:
            logger.error(f"Connection error: {e}")
            return False
    
    def publish_time(self):
        if not self.connected:
            logger.warning("Not connected to MQTT broker, skipping publish")
            return False
        
        try:
            # Get current Unix timestamp
            unix_time = int(time.time())
            
            # Create JSON payload
            payload = {
                "unix_time": unix_time
            }
            
            # Publish to topic
            topic = f"{MQTT_BASE}/time"
            result = self.client.publish(topic, json.dumps(payload), qos=1)
            
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                # Log with human-readable time for verification
                human_time = datetime.fromtimestamp(unix_time).strftime('%Y-%m-%d %H:%M:%S')
                logger.info(f"Published Unix time: {unix_time} ({human_time})")
                return True
            else:
                logger.error(f"Failed to publish message. Return code: {result.rc}")
                return False
                
        except Exception as e:
            logger.error(f"Error publishing time: {e}")
            return False
    
    def run(self):
        logger.info("Starting MQTT Time Publisher...")
        logger.info(f"Broker: {MQTT_HOST}:{MQTT_PORT}")
        logger.info(f"Topic: {MQTT_BASE}/time")
        logger.info(f"Publish interval: {PUBLISH_INTERVAL} seconds")
        
        if not self.connect():
            logger.error("Failed to connect to MQTT broker. Exiting.")
            return 1
        
        try:
            while True:
                if self.connected:
                    self.publish_time()
                else:
                    logger.warning("Connection lost, attempting to reconnect...")
                    if not self.connect():
                        logger.error("Reconnection failed, waiting before retry...")
                        time.sleep(30)
                        continue
                
                # Wait for next publish interval
                time.sleep(PUBLISH_INTERVAL)
                
        except KeyboardInterrupt:
            logger.info("Received interrupt signal, shutting down...")
        except Exception as e:
            logger.error(f"Unexpected error: {e}")
        finally:
            self.client.loop_stop()
            self.client.disconnect()
            logger.info("MQTT Time Publisher stopped")
        
        return 0

def main():
    """Main entry point"""
    publisher = TimePublisher()
    return publisher.run()

if __name__ == "__main__":
    sys.exit(main())