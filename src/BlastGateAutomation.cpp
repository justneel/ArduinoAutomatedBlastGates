#include <Arduino.h>
#include <Servo.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
// #include <EEPROM.h>
#include <stdlib.h>
#include "GateController.h"
#include "GatePins.h"
#include "Constants.h"
#include "Log.h"


void notifyCurrentFlowing();
void broadcastCommand(String cmd);
int currentGateCode();
void checkOtherGates();
void processCommand(String fromId, int gateCode, String command);
void turnOnDustCollector();
void turnOffDustCollector();
double currentAmps();
bool isCurrentFlowing();
void onMessageAvailable();

RF24 radio(CE_PIN, CSN_PIN);

bool currentFlowing = false;

const long UNSET = -1;
long lastBroadcastTime = UNSET;
long lastOnBroadcastReceivedTime = UNSET;
long currentStoppedTime = UNSET;

long id = UNSET;

GateController gateController;

char textBuff[128] = {0};

Servo myservo;
int pos = 0;
void setup() {
  
  Serial.begin(9600);

  radio.begin();
  radio.maskIRQ(1, 1, 0);
  radio.setChannel(CHANNEL);
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(0, myAddress);
  radio.startListening();

  if (USE_FAKE_CURRENT) {
    pinMode(CURRENT_SENSOR_PIN, INPUT_PULLUP);  
  } else {
    pinMode(CURRENT_SENSOR_PIN, INPUT);
  }

  pinMode(CURRENT_SENSOR_PIN, INPUT);
  pinMode(DUST_COLLECTOR_PIN, OUTPUT);
  digitalWrite(DUST_COLLECTOR_PIN, LOW);
  
  gateController.setup();

  for (int i = 0; i < BRANCH_PINS_LENGTH; i++) {
    pinMode(BRANCH_PINS[i], INPUT_PULLUP);
  }

  if (mode == DUST_COLLECTOR) {
    turnOffDustCollector();
  }
  
  // Not doing interrupt since we need to constantly check for current which is an analog signal
  //attachInterrupt(digitalPinToInterrupt(WIRELESS_IRQ_PIN), onMessageAvailable, FALLING);
  
  Serial.println("Starting blast gate automation");
}

void loop() {
  if (mode == MACHINE) {
    if (isCurrentFlowing()) {
      if (!currentFlowing || (lastBroadcastTime + TIME_BETWEEN_ON_BROADCASTS) < millis()) {
        notifyCurrentFlowing();
        lastBroadcastTime = millis();
        currentFlowing = true;
        gateController.openGate();
      }
    } else if (currentFlowing) {
      //notifyCurrentStopped();
      Serial.println("Current has stopped flowing");
      currentFlowing = false;
      currentStoppedTime = millis();
    } else if (closeGateWhenNotInUse && currentStoppedTime != UNSET && (currentStoppedTime + CLOSE_GATE_DELAY) < millis()) {
      gateController.closeGate();
      currentStoppedTime = UNSET;
    }
  } else if (mode == DUST_COLLECTOR) {
    if (lastOnBroadcastReceivedTime != UNSET && (lastOnBroadcastReceivedTime + DUST_COLLECTOR_TURN_OFF_DELAY) < millis()) {
      lastOnBroadcastReceivedTime = UNSET;
      turnOffDustCollector();
    }
  } else if (mode == BRANCH_GATE) {
    if (lastOnBroadcastReceivedTime != UNSET && (lastOnBroadcastReceivedTime + CLOSE_BRANCH_GATE_DELAY) < millis()) {
      Serial.println("Closing branch gate");
      lastOnBroadcastReceivedTime = UNSET;
      gateController.closeGate();
    }
  }

  gateController.onLoop();
  checkOtherGates();

  if (SLOW_DOWN_LOOP) {
    delay(100);
  }
}

void notifyCurrentFlowing() {
  if (id == UNSET) {
    randomSeed(millis());
    id = abs(random(2147483600));
  }
  String cmd = String(id);
  cmd.concat(COMMAND_DELIM);
  cmd.concat(String(currentGateCode()));
  cmd.concat(COMMAND_DELIM);
  cmd.concat(ACTIVE_COMMAND);
  Serial.println("Notifying others I am on");

  broadcastCommand(cmd);
}

//
//void notifyCurrentStopped() {
//  String cmd = String(id);
//  cmd.concat(COMMAND_DELIM);
//  cmd.concat(INACTIVE_COMMAND);
//
//  Serial.println("Notifying others I am off");
//  broadcastCommand(cmd);
//}

void broadcastCommand(String cmd) {
  Serial.print("Broadcasting command: ");
  Serial.println(cmd);

  radio.openWritingPipe(sendAddress);
  radio.stopListening();
  radio.write(cmd.c_str(), cmd.length());
  radio.startListening();
}

int currentGateCode() {
  int value = 0;
  for (int i = 0; i < BRANCH_PINS_LENGTH; i++) {
    value *= 2;
    if (digitalRead(BRANCH_PINS[i]) == LOW) {
      value++;
    }
  }
  return value;
}

void onMessageAvailable() {
  bool b1, b2, b3;
  // Clear out the interrupt
  radio.whatHappened(b1, b2, b3);
}

void checkOtherGates() {
  if (radio.available()) {
    radio.read(&textBuff, sizeof(textBuff));
    String received = String(textBuff);
    if (received.length() == 0) {
      return;
    }
    Serial.print("Received: ");
    Serial.println(received);
    // serial.printf("Received: %s");

    int indexOfDelim = received.indexOf(COMMAND_DELIM);
    if (indexOfDelim == -1) {
      Serial.println("Could not find first delim");
      return;
    }
    String fromId = received.substring(0, indexOfDelim);
    int gateDelimIndex = received.indexOf(COMMAND_DELIM, indexOfDelim+1);
    if (gateDelimIndex == -1) {
      Serial.println("Could not find second delim");
      return;
    }
    String gateCode = received.substring(indexOfDelim + 1, gateDelimIndex);
    String remoteStatus = received.substring(gateDelimIndex + 1);
    processCommand(fromId, gateCode.toInt(), remoteStatus);
  }
}

void processCommand(String fromId, int gateCode, String command) {
  if (fromId.equals(String(id))) {
    Serial.println("Received id was same as my own id.  Ignoring.");
    return;
  }
  if (command.equals(ACTIVE_COMMAND_STRING)) {
    if (mode == DUST_COLLECTOR) {
      turnOnDustCollector();
      lastOnBroadcastReceivedTime = millis();
    } else if (mode == MACHINE) {
      if (!currentFlowing) {
        Serial.println("Remote is on, and I am not.  Closing my gate.");
        gateController.closeGate();
      } else {
        Serial.println("Current is flowing, so not closing my gate");
      }
    } else if (mode == BRANCH_GATE) {
      int myCode = currentGateCode();
      // Serial.print("Got a on command from a branch: ");
      // Serial.print(gateCode);
      // Serial.print(" compared to my code: ");
      // Serial.println(myCode);
      if ((gateCode & myCode) != 0) {
        Serial.println("I matched incoming code.  Opening my gate");
        gateController.openGate();
        lastOnBroadcastReceivedTime = millis();
      } else {
        Serial.print("Got a on command from a branch that was not mine (");
        Serial.print(gateCode);
        Serial.print("/");
        Serial.print(myCode);
        Serial.println(")");
      }
    }
  } else {
    Serial.print("Not an active command.  Ignoring: ");
    Serial.println(command);
  }
}

void turnOnDustCollector() {
  Serial.println("Turning on dust collector");
  digitalWrite(DUST_COLLECTOR_PIN, HIGH);
}

void turnOffDustCollector() {
  Serial.println("Turning off dust collector");
  digitalWrite(DUST_COLLECTOR_PIN, LOW);
}

// Derived from: https://arduino.stackexchange.com/questions/19301/acs712-sensor-reading-for-ac-current
double currentAmps() {
  if (USE_FAKE_CURRENT) {
    if (analogRead(CURRENT_SENSOR_PIN) > 512) {
      return MIN_CURRENT_TO_ACTIVATE + 1;
    }
    return 0;
  }
  int rVal = 0;
  int maxVal = 0;
  int minVal = 1023;
  int sampleDuration = 100; // 100ms

  uint32_t startTime = millis();
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

bool isCurrentFlowing() {
  // double current = currentAmps();
  // Serial.print("Current flowing: ");
  // Serial.println(current);
  return currentAmps() >= MIN_CURRENT_TO_ACTIVATE;
}

