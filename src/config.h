// Everything you are likely to change when building your own unit.
#pragma once

#include "rotator_core.h"

// --- Wi-Fi access point (phone control) -----------------------------------
// CHANGE THE PASSWORD before use; it must be at least 8 characters.
#define WIFI_SSID "antenna-rotator"
#define WIFI_PASSWORD "change-me-1234"
// 2.4 GHz channel (1-13). Move it if the drone link or the rotator misbehaves.
#define WIFI_CHANNEL 1

// --- Pins (ESP32 DevKit) ---------------------------------------------------
#define PIN_SERVO_PAN 18
#define PIN_SERVO_TILT 19

// Optional wired joystick at the base (KY-023 or similar). Leave at 0 unless
// one is actually connected: floating analog pins would move the antenna.
#ifndef HAS_JOYSTICK
#define HAS_JOYSTICK 0
#endif
#define PIN_JOY_X 34     // ADC1 only; ADC2 pins do not work while Wi-Fi is on
#define PIN_JOY_Y 35
#define PIN_JOY_BUTTON 32  // press = STOP

// --- Mechanics ---------------------------------------------------------------
// Defaults are for 270-degree digital servos (500-2500 us), e.g. DS3235SG-270.
// For a 180-degree servo set servoSpanDeg to 180 and tighten the limits.
// Fields: minDeg, maxDeg, maxSpeedDps, accelDps2, servoSpanDeg, minUs, maxUs,
//         invert, trimDeg
inline rotator::Config rotatorConfig() {
  rotator::AxisConfig pan{-130, 130, 30, 60, 270, 500, 2500, false, 0};
  rotator::AxisConfig tilt{-15, 60, 20, 40, 270, 500, 2500, false, 0};
  return rotator::Config{pan, tilt, 0.6f};
}
