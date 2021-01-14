#ifndef Gate_pins_h
#define Gate_pins_h

#include <Arduino.h>

const int CURRENT_SENSOR_PIN = A5;

const int DUST_COLLECTOR_PIN = A2;

// const int WIRELESS_IRQ_PIN = 2;

const int CE_PIN = 9;
const int CSN_PIN = 10;

const int BRANCH_PINS[] = {3, 4, 5, 6};
const int BRANCH_PINS_LENGTH = 4;

const int SERVO_PIN = 7;
const int SERVO_POWER_PIN = 8;
const int CLOSED_POT_PIN = A4;
const int OPEN_POT_PIN = A3; 

#endif