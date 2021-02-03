#include <Arduino.h>
#include <Servo.h>

#include <stdlib.h>
#include "GateController.h"
#include "GatePins.h"
#include "Constants.h"
#include "RadioController.h"
#include "Log.h"
#include <printf.h>
#include <limits.h>

void notifyCurrentFlowing();

void checkOtherGates();
void processCommand(const Payload &payload);
void turnOnDustCollector();
void turnOffDustCollector();
double currentAmps();


RadioController radioController;
GateController gateController;

bool currentFlowing = false;
bool dustCollectorOn = false;

unsigned long lastBroadcastTime = VALUE_UNSET;
unsigned long lastOnBroadcastReceivedTime = VALUE_UNSET;
unsigned long currentStoppedTime = VALUE_UNSET;

void setup() {
  Serial.begin(9600);
  printf_begin();
  Serial.println(" ");
  Serial.println("------------");
  Serial.println(" ");
  Serial.println("Starting setup");
  
  // analogReference(INTERNAL);

  if (USE_FAKE_CURRENT) {
    pinMode(CURRENT_SENSOR_PIN, INPUT_PULLUP);  
  } else {
    pinMode(CURRENT_SENSOR_PIN, INPUT);
  }

  pinMode(DUST_COLLECTOR_PIN, OUTPUT);
  digitalWrite(DUST_COLLECTOR_PIN, LOW);
  
  gateController.setup();
  radioController.setup();


  if (mode == DUST_COLLECTOR) {
    turnOffDustCollector();
  }

  Serial.print("Starting blast gate automation in mode: ");
  switch (mode) {
    case MACHINE:
      Serial.println("MACHINE");
      break;
    case DUST_COLLECTOR:
      Serial.println("DUST_COLLECTOR");
      break;
    case BRANCH_GATE:
      Serial.println("BRANCH_GATE");
      break;
  }

  radioController.broadcastCommand(HELLO_WORLD);
}


void loop() {
  radioController.onLoop();

  if (mode == MACHINE) {
    double current = currentAmps();
    if (current >= MIN_CURRENT_TO_ACTIVATE) {
      if (!currentFlowing || (lastBroadcastTime + TIME_BETWEEN_ON_BROADCASTS) < millis()) {
        Serial.print("Current is flowing: ");
        Serial.print(current);
        Serial.print("   ");
        lastBroadcastTime = millis();
        currentFlowing = true;
        gateController.openGate();
        notifyCurrentFlowing();
      }
    } else if (currentFlowing) {
      Serial.println("Current has stopped flowing");
      currentFlowing = false;
      currentStoppedTime = millis();
      radioController.broadcastCommand(NO_LONGER_RUNNING, false);
    } else if (closeGateWhenNotInUse && currentStoppedTime != VALUE_UNSET && (currentStoppedTime + CLOSE_GATE_DELAY) < millis()) {
      gateController.closeGate();
      currentStoppedTime = VALUE_UNSET;
    }
  } else if (mode == DUST_COLLECTOR) {
    if (lastOnBroadcastReceivedTime != VALUE_UNSET && (lastOnBroadcastReceivedTime + DUST_COLLECTOR_TURN_OFF_DELAY) < millis()) {
      lastOnBroadcastReceivedTime = VALUE_UNSET;
      turnOffDustCollector();
    }
  } else if (mode == BRANCH_GATE) {
    if (lastOnBroadcastReceivedTime != VALUE_UNSET && (lastOnBroadcastReceivedTime + CLOSE_BRANCH_GATE_DELAY) < millis()) {
      Serial.println("Closing branch gate");
      lastOnBroadcastReceivedTime = VALUE_UNSET;
      gateController.closeGate();
    }
  }

  gateController.onLoop();
  checkOtherGates();

  if (SLOW_DOWN_LOOP) {
    delay(300);
  }
}

void notifyCurrentFlowing() {
  radioController.broadcastCommand(RUNNING, true);
}

void checkOtherGates() {
  Payload received;
  if (radioController.getMessage(received)) {
    processCommand(received);
  }
}

void processCommand(const Payload &payload) {
  
  if (payload.command == RUNNING) {
    if (mode == DUST_COLLECTOR) {
      radioController.sendAck(payload);
      if (!dustCollectorOn) {
        if (payload.gateCode != 0) {
          delay(DUST_COLLECTOR_ON_DELAY_BRANCH);
        }
        turnOnDustCollector();
      }
      lastOnBroadcastReceivedTime = millis();
    } else if (mode == MACHINE) {
      if (!currentFlowing) {
        if (!gateController.isClosed()) {
          Serial.println("Remote is on, and I am not.  Closing my gate.");
          gateController.closeGate();
        }
      } else {
        Serial.println("Current is flowing, so not closing my gate");
      }
    } else if (mode == BRANCH_GATE) {
      int myCode = radioController.currentGateCode();
      if ((payload.gateCode & myCode) != 0) {
        if (!gateController.isOpen()) {
          Serial.println("I matched incoming code.  Opening my gate");
          gateController.openGate();
        }
        lastOnBroadcastReceivedTime = millis();
      } else {
        Serial.print("Got a on command from a branch that was not mine (");
        Serial.print(payload.gateCode);
        Serial.print("/");
        Serial.print(myCode);
        Serial.println(")");
      }
    }
  } else if (payload.command == NO_LONGER_RUNNING) {
    // no action needed
  } else if (payload.command == HELLO_WORLD) {
    // Lets welcome our new guest.
    radioController.broadcastCommand(WELCOME);
  } else if (payload.command == WELCOME) {
    // Do nothing
  } else {
    Serial.print("Unknown");
  }
}

void turnOnDustCollector() {
  dustCollectorOn = true;
  Serial.println("Turning on dust collector");
  digitalWrite(DUST_COLLECTOR_PIN, HIGH);
}

void turnOffDustCollector() {
  dustCollectorOn = false;
  Serial.println("Turning off dust collector");
  digitalWrite(DUST_COLLECTOR_PIN, LOW);
}

// Derived from: https://arduino.stackexchange.com/questions/19301/acs712-sensor-reading-for-ac-current
double currentAmps() {
  if (USE_FAKE_CURRENT) {
    if (analogRead(CURRENT_SENSOR_PIN) > 512) {
      return 0;
    }
    return MIN_CURRENT_TO_ACTIVATE + 5;
  }

  int rVal = 0;
  int maxVal = 0;
  int minVal = 1023;
  unsigned long sampleDuration = 100; // 100ms

  unsigned long startTime = millis();
  // take samples for 100ms
  while ((millis() - startTime) < sampleDuration)
  {
    rVal = analogRead(CURRENT_SENSOR_PIN);
    if (rVal > maxVal)
      maxVal = rVal;

    if (rVal < minVal)
      minVal = rVal;
  }

  // Subtract min from max to determine the peak to peak range
  // 1023 = the max value we'll get on the input (1024, zero indexed)
  // 5.0 = 5v total on adc input
  double volt = ((maxVal - minVal) * (5.0 / 1023.0));

  // div by 2 is to calculate RMS from peak to peak
  // 0.35355 is factor to calculate RMS from peak to peak
  // see http://www.learningaboutelectronics.com/Articles/Voltage-rms-calculator.php
  double voltRMS = volt * 0.35355;

  // x 1000 to convert volts to millivolts
  // divide by the number of millivolts per amp to determine amps measured
  // the 20A module 100 mv/A (so in this case ampsRMS = voltRMS
  double ampsRMS = (voltRMS * 1000) / 66;

  return ampsRMS;
}

