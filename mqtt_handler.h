/*
  mqtt_handler.h - MQTT connection management and message handling
*/

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <PubSubClient.h>

// MQTT client (defined in main .ino)
extern PubSubClient mqtt;

// Ensure MQTT connection with exponential backoff
void ensureMqtt();

// MQTT message callback - routes to appropriate handlers
void mqttCallback(char* topic, byte* payload, unsigned int len);

// Trigger internal "MERRY CHRISTMAS" scrolling notification
void triggerChristmasScroll();

#endif // MQTT_HANDLER_H
