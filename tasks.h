/*
  tasks.h - FreeRTOS task functions (render + network)
*/

#ifndef TASKS_H
#define TASKS_H

// Render task - Core 1: display and UI updates
void renderTask(void *pvParameters);

// Network task - Core 0: WiFi, MQTT, weather
void networkTask(void *pvParameters);

#endif // TASKS_H
