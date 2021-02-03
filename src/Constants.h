#ifndef Constants_h
#define Constants_h
#include <Arduino.h>
#include <RF24.h>

enum Mode {
  MACHINE,
  DUST_COLLECTOR,
  BRANCH_GATE
};

const Mode mode = MACHINE;
// const Mode mode = BRANCH_GATE;
// const Mode mode = DUST_COLLECTOR;

const bool closeGateWhenNotInUse = true;

const bool SERIAL_CALIBRATION = true;

const bool USE_FAKE_CURRENT = true; // NON-DEBUG = false

const bool SLOW_DOWN_LOOP = false; // NON-DEBUG = false

// const unsigned long DELAY_BETWEEN_SERVO_STEPS_MS = 10; // DO NOT PUSH
const unsigned long DELAY_BETWEEN_SERVO_STEPS_MS = 5;

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
const unsigned long TIME_TO_CALIBRATE_MS = 7000;

const unsigned long DUST_COLLECTOR_ID = 123456789;

const uint64_t myAddress =  0xDEADBEEFAB;
const uint64_t sendAddress = myAddress;
const uint64_t ackAddress = 0xDEADBEEFAD;

const uint8_t CHANNEL = 124;

const unsigned long VALUE_UNSET = 0;

#endif