/*
  chime.cpp - Hourly chime implementation

  Soft two-note chime using PWM duty-cycle fading.
  A piezo driven by ledcWriteTone produces harsh square waves.
  By rapidly stepping the duty cycle down after each note starts,
  we get a "ding" that decays instead of a sustained buzz.
*/

#include "chime.h"
#include "hw_config.h"

// Fade-out parameters
static const uint8_t FADE_STEPS = 8;       // number of fade steps per note
static const unsigned long NOTE_MS = 80;    // total note duration before fade
static const unsigned long FADE_MS = 120;   // total fade-out duration
static const unsigned long GAP_MS  = 80;    // silence between notes
static const uint16_t NOTE1_HZ = 523;      // C4
static const uint16_t NOTE2_HZ = 659;      // E4

static uint8_t fadeStep = 0;

// Duty values: start loud, taper off quickly (8-bit resolution)
static const uint8_t dutyTable[FADE_STEPS] = {
  180, 120, 80, 50, 30, 16, 8, 0
};

static void startNote(uint16_t hz) {
  ledcWriteTone(BUZZER_PIN, hz);
  ledcWrite(BUZZER_PIN, dutyTable[0]);
}

static void stopNote() {
  ledcWriteTone(BUZZER_PIN, 0);
  ledcWrite(BUZZER_PIN, 0);
}

void triggerChime() {
  chimePhase = 1;
  chimePhaseMs = millis();
  fadeStep = 0;
  startNote(NOTE1_HZ);
}

void updateChime() {
  if (chimePhase == 0) return;
  unsigned long elapsed = millis() - chimePhaseMs;

  switch (chimePhase) {
    case 1:  // note 1 — sustain then fade
      if (elapsed < NOTE_MS) break;  // sustain at full duty
      if (elapsed < NOTE_MS + FADE_MS) {
        uint8_t step = ((elapsed - NOTE_MS) * FADE_STEPS) / FADE_MS;
        if (step >= FADE_STEPS) step = FADE_STEPS - 1;
        if (step != fadeStep) {
          fadeStep = step;
          ledcWrite(BUZZER_PIN, dutyTable[fadeStep]);
        }
      } else {
        stopNote();
        chimePhase = 2;
        chimePhaseMs = millis();
      }
      break;

    case 2:  // gap
      if (elapsed >= GAP_MS) {
        fadeStep = 0;
        startNote(NOTE2_HZ);
        chimePhase = 3;
        chimePhaseMs = millis();
      }
      break;

    case 3:  // note 2 — sustain then fade
      if (elapsed < NOTE_MS) break;
      if (elapsed < NOTE_MS + FADE_MS) {
        uint8_t step = ((elapsed - NOTE_MS) * FADE_STEPS) / FADE_MS;
        if (step >= FADE_STEPS) step = FADE_STEPS - 1;
        if (step != fadeStep) {
          fadeStep = step;
          ledcWrite(BUZZER_PIN, dutyTable[fadeStep]);
        }
      } else {
        stopNote();
        chimePhase = 0;
      }
      break;
  }
}
