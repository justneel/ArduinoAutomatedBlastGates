#include <Arduino.h>
#include <Servo.h>

#include <stdlib.h>
#include "GateController.h"
#include "GatePins.h"
#include "Constants.h"
#include "RadioController.h"
#include <limits.h>
#include "StatusController.h"
#include "Ids.h"

void checkOtherGates();
void processCommand(const Payload &payload);
void turnOnDustCollector();
void turnOffDustCollector();
double currentAmps();

Ids *ids;
StatusController *statusController;
RadioController *radioController;
GateController *gateController;

bool currentFlowing = false;
bool dustCollectorOn = false;

unsigned long lastBroadcastTime = VALUE_UNSET;
unsigned long lastOnBroadcastReceivedTime = VALUE_UNSET;
unsigned long currentStoppedTime = VALUE_UNSET;

void setup() {
  Serial.begin(9600);
  Serial.println(" ");
  Serial.println("------------");
  Serial.println(" ");
  Serial.println("Starting setup");
  
  ids = new Ids();
  statusController = new StatusController();
  radioController = new RadioController(*statusController, *ids);
  gateController = new GateController(*statusController, *ids);

  if (USE_FAKE_CURRENT) {
    pinMode(CURRENT_SENSOR_PIN, INPUT_PULLUP);  
  } else {
    pinMode(CURRENT_SENSOR_PIN, INPUT);
  }

// if (MODE_VIA_PIN) {
//     pinMode(MODE_PIN, INPUT_PULLUP);
//     if (digitalRead(MODE_PIN) == HIGH) {
//       Serial.println("Reassigning to Dust collector");
//       mode = DUST_COLLECTOR;
//     } else {
//       Serial.println("Reassigning to MACHINE");
//       mode = MACHINE;
//     }
//   }
  
  ids->setup();
  statusController->setup();
  gateController->setup();
  radioController->setup();

  if (mode == DUST_COLLECTOR) {
    pinMode(DUST_COLLECTOR_PIN, OUTPUT);
    digitalWrite(DUST_COLLECTOR_PIN, LOW);
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

  radioController->broadcastCommand(HELLO_WORLD);
}


void loop() {
  if (mode == MACHINE) {
    double current = currentAmps();
    if (current >= MIN_CURRENT_TO_ACTIVATE) {
      if (!currentFlowing || (lastBroadcastTime + TIME_BETWEEN_ON_BROADCASTS) < millis()) {
        Serial.print("Current is flowing: ");
        Serial.print(current);
        Serial.print("   ");
        lastBroadcastTime = millis();
        currentFlowing = true;
        gateController->openGate();
        statusController->setGateStatus(true);
        statusController->onSystemActive();
        radioController->broadcastCommand(RUNNING, true);
      }
    } else if (currentFlowing) {
      Serial.println("Current has stopped flowing");
      currentFlowing = false;
      currentStoppedTime = millis();
      statusController->setGateStatus(false);
      radioController->broadcastCommand(NO_LONGER_RUNNING, false);
    } else if (closeGateWhenNotInUse && currentStoppedTime != VALUE_UNSET && (currentStoppedTime + CLOSE_GATE_DELAY) < millis()) {
      gateController->closeGate();
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
      gateController->closeGate();
    }
  }

  radioController->onLoop();
  gateController->onLoop();
  statusController->onLoop();

  checkOtherGates();

  if (SLOW_DOWN_LOOP) {
    delay(300);
  }
}

void checkOtherGates() {
  Payload received;
  if (radioController->getMessage(received)) {
    processCommand(received);
  }
}

void processCommand(const Payload &payload) {
  if (payload.command == RUNNING) {
    statusController->onSystemActive();
    if (mode == DUST_COLLECTOR) {
      if (!dustCollectorOn) {
        if (payload.gateCode != 0) {
          Serial.println("Delaying turning on dust collector until gates open");
          delay(DUST_COLLECTOR_ON_DELAY_BRANCH);
        }
        turnOnDustCollector();
      }
      lastOnBroadcastReceivedTime = millis();
    } else if (mode == MACHINE) {
      if (!currentFlowing) {
        if (!gateController->isClosed()) {
          Serial.println("Remote is on, and I am not.  Closing my gate.");
          gateController->closeGate();
        }
      } else {
        Serial.println("Current is flowing, so not closing my gate");
      }
    } else if (mode == BRANCH_GATE) {
      int myCode = ids->currentGateCode();
      if ((payload.gateCode & myCode) != 0) {
        if (!gateController->isOpen()) {
          Serial.println("I matched incoming code.  Opening my gate");
          gateController->openGate();
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
    radioController->broadcastCommand(WELCOME);
    statusController->onSystemActive();
  } else if (payload.command == WELCOME) {
    // Do nothing
  } else if (payload.command == ACK) {
    // Do nothing
  } else {
    Serial.println("Unknown command");
  }
}

void turnOnDustCollector() {
  dustCollectorOn = true;
  Serial.println("Turning on dust collector");
  digitalWrite(DUST_COLLECTOR_PIN, HIGH);
  statusController->setGateStatus(true);
}

void turnOffDustCollector() {
  dustCollectorOn = false;
  Serial.println("Turning off dust collector");
  statusController->setGateStatus(false);
  digitalWrite(DUST_COLLECTOR_PIN, LOW);
}

// Derived from: https://arduino.stackexchange.com/questions/19301/acs712-sensor-reading-for-ac-current
double currentAmps() {
  if (USE_FAKE_CURRENT) {
    bool isHigh = analogRead(CURRENT_SENSOR_PIN) > 512;
    double onCurrent = MIN_CURRENT_TO_ACTIVATE + 5;
    if (FAKE_CURRENT_DEFAULT_ON) {
      if (isHigh) {
        return onCurrent;
      } else {
        return 0;
      }
    } else {
      if (isHigh) {
        return 0;
      } else {
        return onCurrent;
      }
    }
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

