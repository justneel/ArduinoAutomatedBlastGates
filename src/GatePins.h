#ifndef Gate_pins_h
#define Gate_pins_h

#include <Arduino.h>

// Pins
const int CURRENT_SENSOR_PIN = A5;

const int DUST_COLLECTOR_PIN = A1;

const int WIRELESS_IRQ_PIN = 2;

const int CE_PIN = 9;
const int CSN_PIN = 10;

const int BRANCH_PINS[] = {1, 0, 2, 3, 4, 5, 6, 7};
const int BRANCH_PINS_LENGTH = 8;

const int SERVO_PIN = A0;
const int SERVO_POWER_PIN = A1;
const int CLOSED_POT_PIN = A4;
const int OPEN_POT_PIN = A3;

#endif