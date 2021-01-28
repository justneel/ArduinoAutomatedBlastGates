#ifndef Constants_h
#define Constants_h
#include <Arduino.h>

enum Mode {
  MACHINE,
  DUST_COLLECTOR,
  BRANCH_GATE
};

const Mode mode = MACHINE;
// const Mode mode = BRANCH_GATE;

const bool closeGateWhenNotInUse = true;

const bool ALLOW_CALIBRATION = true; // NON-DEBUG = true

const bool USE_FAKE_CURRENT = true; // NON-DEBUG = false

const bool SLOW_DOWN_LOOP = true; // NON-DEBUG = false

const unsigned long DELAY_BETWEEN_SERVO_STEPS_MS = 10; // DO NOT PUSH

const double MIN_CURRENT_TO_ACTIVATE = 2.0;

const unsigned long TIME_BETWEEN_ON_BROADCASTS = 3000;

/**
 * When we are the dust collector, this is the delay after the last 
 * machine turns off before we turn off the dust collector.
 */
const unsigned long DUST_COLLECTOR_TURN_OFF_DELAY = TIME_BETWEEN_ON_BROADCASTS * 2;

const unsigned long DUST_COLLECTOR_ON_DELAY_BRANCH = 500;

/**
 * If we are closing the gate when not in use, this is the 
 * delay after the machine turns off before closing the gate. 
 */
const unsigned long CLOSE_GATE_DELAY = DUST_COLLECTOR_TURN_OFF_DELAY + 3000;

const unsigned long CLOSE_BRANCH_GATE_DELAY = DUST_COLLECTOR_TURN_OFF_DELAY + 5000;

/**
 * When we enter gate calibration mode, this is how long we default stay in that mode
 * before returning to normal operations.
 */
const unsigned long TIME_TO_CALIBRATE_MS = 3000;

const long ANLOG_MAX_VALUE = 1023;
const long MAX_ROTATION = 180;

// THe analog pin will float slightly.  We actually don't care unless the
// new value would cause a change in the servo position which is defined in
// single degrees.
// const int ANALOG_FLOAT_AMOUNT = map(1, 0, MAX_ROTATION, 0, ANLOG_MAX_VALUE);
const int BEGIN_CALIBRATION_CHANGE_AMOUNT = map(3, 0, MAX_ROTATION, 0, ANLOG_MAX_VALUE);
const int IN_CALIBRATION_ANALOG_FLOAT_AMOUNT = map(1, 0, MAX_ROTATION, 0, ANLOG_MAX_VALUE);
// const int ANALOG_FLOAT_AMOUNT = 5;

// const bool USE_POWER_PIN = false;

const byte myAddress[6] = "Tools";
const byte sendAddress[6] = "Tools";
const byte ackAddress[6] = "ToolA";

const uint8_t CHANNEL = 83;

#endif