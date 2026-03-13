/*
  chime.h - Hourly chime (non-blocking two-note state machine)
*/

#ifndef CHIME_H
#define CHIME_H

#include <Arduino.h>

// Chime state
extern uint8_t chimePhase;
extern unsigned long chimePhaseMs;

// Start the two-note chime sequence
void triggerChime();

// Update chime state machine (call from render loop)
void updateChime();

#endif // CHIME_H
