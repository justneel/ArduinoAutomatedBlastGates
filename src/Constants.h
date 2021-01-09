#ifndef Constants_h
#define Constants_h
#include <Arduino.h>

const bool closeGateWhenNotInUse = true;
/**
 * If we are closing the gate when not in use, this is the 
 * delay after the machine turns off before closing the gate. 
 */
const long CLOSE_GATE_DELAY = 3000; // DO NOT PUSH

const long DELAY_BETWEEN_SERVO_STEPS_MS = 20; // DO NOT PUSH

const double MIN_CURRENT_TO_ACTIVATE = 2.0;

const unsigned long TIME_BETWEEN_ON_BROADCASTS = 3000;

/**
 * When we are the dust collector, this is the delay after the last 
 * machine turns off before we turn off the dust collector.
 */
const long TURN_OFF_DELAY = 10000;

const bool USE_FAKE_CURRENT = true;

const unsigned long TIME_TO_CALIBRATE_MS = 10000;
const int ANALOG_FLOAT_AMOUNT = 2;

const byte myAddress[6] = "Tools";
const byte sendAddress[6] = "Tools";
const uint8_t CHANNEL = 83;

const int ID_BYTES = 8;
const int DELIM_BYTES = 1;
const int PAYLOAD_SIZE = 32;

const char ACTIVE_COMMAND[PAYLOAD_SIZE - ID_BYTES - DELIM_BYTES] =   "IAmRunning";
const String ACTIVE_COMMAND_STRING = String(ACTIVE_COMMAND);

const char IC_CHECK_COMMAND[PAYLOAD_SIZE - ID_BYTES - DELIM_BYTES] =   "WhatYourId";
const char IC_RESPONSE_COMMAND[PAYLOAD_SIZE - ID_BYTES - DELIM_BYTES] =   "ThisIsMyId";

//const char INACTIVE_COMMAND[PAYLOAD_SIZE - ID_BYTES - DELIM_BYTES] = "NotRunningNow";
//const String INACTIVE_COMMAND_STRING = String(INACTIVE_COMMAND);

const String COMMAND_DELIM = "_";
#endif