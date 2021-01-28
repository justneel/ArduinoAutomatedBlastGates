#include <Arduino.h>
#include <Servo.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <stdlib.h>
#include "GateController.h"
#include "GatePins.h"
#include "Constants.h"
#include "Log.h"
#include <printf.h>
#include <limits.h>

const unsigned long VALUE_UNSET = LONG_MAX;
const rf24_datarate_e RADIO_DATA_RATE = RF24_1MBPS;
const rf24_pa_dbm_e RADIO_POWER_LEVEL = RF24_PA_LOW;

enum Command {
    UNKNOWN,
    RUNNING,
    NO_LONGER_RUNNING,
    HELLO_WORLD, // Debugging message sent out when a machine first comes online
    WELCOME, // Response back from the HELLO_WORLD
};

struct Payload {
  unsigned long messageId = 0;
  unsigned long id = 0;
  unsigned int gateCode = 0;
  Command command = UNKNOWN;
  char debugMessage[20] = {0};
  // unsigned int debugMessageLength;
};

void configureRadio();
void notifyCurrentFlowing();
void broadcastCommand(Payload &payload);
unsigned int currentGateCode();
void checkOtherGates();
void processCommand(const Payload &payload);
void turnOnDustCollector();
void turnOffDustCollector();
double currentAmps();
bool isCurrentFlowing();

RF24 radio(CE_PIN, CSN_PIN);
GateController gateController;

bool currentFlowing = false;
bool dustCollectorOn = false;

unsigned long lastBroadcastTime = VALUE_UNSET;
unsigned long lastOnBroadcastReceivedTime = VALUE_UNSET;
unsigned long currentStoppedTime = VALUE_UNSET;

unsigned long id = VALUE_UNSET;
unsigned long currentMessageId = 0;

void setup() {
  Serial.begin(9600);
  printf_begin();
  Serial.println(" ");
  Serial.println("------------");
  Serial.println(" ");
  Serial.println("Starting setup");
  
  analogReference(INTERNAL);

  // Let the capacitor charge up before turning on the radio.
  // No idea if this will actually do anything.
  delay(100);
  configureRadio();

  if (USE_FAKE_CURRENT) {
    pinMode(CURRENT_SENSOR_PIN, INPUT_PULLUP);  
  } else {
    pinMode(CURRENT_SENSOR_PIN, INPUT);
  }

  pinMode(DUST_COLLECTOR_PIN, OUTPUT);
  digitalWrite(DUST_COLLECTOR_PIN, LOW);
  
  gateController.setup();

  for (int i = 0; i < BRANCH_PINS_LENGTH; i++) {
    pinMode(BRANCH_PINS[i], INPUT_PULLUP);
  }

  if (mode == DUST_COLLECTOR) {
    turnOffDustCollector();
  }

  randomSeed(analogRead(A0));

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

  Payload send;
  send.id = id;
  send.gateCode = currentGateCode();
  send.command = HELLO_WORLD;
  memcpy(send.debugMessage, "IAmHere", 7);
  broadcastCommand(send);
}

void print(const Payload &payload) {
  Serial.print("Payload {");
  Serial.print(" messageId=");
  Serial.print(payload.messageId);
  Serial.print(" id=");
  if (payload.id == VALUE_UNSET) {
    Serial.print("UNSET");
  } else {
    Serial.print(payload.id);
  }
  Serial.print(" gateCode=");
  Serial.print(payload.gateCode);
  Serial.print(" command=");
  switch (payload.command) {
    case RUNNING:
      Serial.print("RUNNING");
      break;
    case NO_LONGER_RUNNING:
      Serial.print("NO_LONGER_RUNNING");
      break;
    case HELLO_WORLD:
      Serial.print("HELLO_WORLD");
      break;
    case WELCOME:
      Serial.print("WELCOME");
      break;
    default:
      Serial.print("UNKNOWN");
  }
  Serial.print(" debug=");
  Serial.print(payload.debugMessage);
  Serial.print(" ");
  Serial.print(" }");
}

void println(const Payload &payload) {
  print(payload);
  Serial.println(" ");
}

void configureRadio() {
  radio.failureDetected = false;
  while (!radio.begin() || !radio.isChipConnected()) {
    Serial.print(millis() / 1000);
    Serial.println(" Waiting for radio to start");
    // radio.printDetails();
    delay(1000);
  }
  radio.setPALevel(RADIO_POWER_LEVEL);
  radio.setChannel(CHANNEL);
  radio.setAutoAck(false);
  if (!radio.setDataRate(RADIO_DATA_RATE)) {
    Serial.println("Could not set the data rate");
    radio.failureDetected = true;
  }
  radio.setCRCLength(RF24_CRC_8);
  radio.openReadingPipe(0, myAddress);
  radio.startListening();
  // radio.printDetails();
}

void loop() {
  if (radio.failureDetected 
      || radio.getDataRate() != RADIO_DATA_RATE 
      || radio.getPALevel() != RADIO_POWER_LEVEL) {
    Serial.println("Radio failure detected.");
    radio.failureDetected = true;
    configureRadio();
  }
  if (mode == MACHINE) {
    if (isCurrentFlowing()) {
      if (!currentFlowing || (lastBroadcastTime + TIME_BETWEEN_ON_BROADCASTS) < millis()) {
        notifyCurrentFlowing();
        lastBroadcastTime = millis();
        currentFlowing = true;
        gateController.openGate();
      }
    } else if (currentFlowing) {
      Serial.println("Current has stopped flowing");
      currentFlowing = false;
      currentStoppedTime = millis();

      Payload send;
      send.id = id;
      send.gateCode = currentGateCode();
      send.command = NO_LONGER_RUNNING;
      memcpy(send.debugMessage, "MachineStopped", 14);
      broadcastCommand(send);
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
  if (id == VALUE_UNSET) {
    randomSeed(millis());
    id = abs(random(2147483600));
  }
  Payload send;
  send.id = id;
  send.gateCode = currentGateCode();
  send.command = RUNNING;
  memcpy(send.debugMessage, "CurentFlowing", 13);
  broadcastCommand(send);
}

void broadcastCommand(Payload &payload) {
  payload.messageId = ++currentMessageId;
  if (currentMessageId >= (LONG_MAX - 1)) {
    currentMessageId = 0;
  }
  Serial.print(millis() / 1000);
  Serial.print(" Broadcasting: ");
  println(payload);

  radio.openWritingPipe(sendAddress);
  radio.stopListening();
  radio.write(&payload, sizeof(payload));
  radio.startListening();
}

unsigned int currentGateCode() {
  unsigned int value = 0;
  for (int i = 0; i < BRANCH_PINS_LENGTH; i++) {
    value *= 2;
    if (digitalRead(BRANCH_PINS[i]) == LOW) {
      value++;
    }
  }
  return value;
}

void checkOtherGates() {
  if (radio.available()) {
    Payload received;
    radio.read(&received, sizeof(Payload));

    if (received.messageId == 0) {
      // Received a blank message.  Just ignore.
      return;
    }
    
    Serial.print(millis() / 1000);
    Serial.print(" Received: ");
    println(received);

    processCommand(received);
  }
}

void processCommand(const Payload &payload) {
  if (payload.id == id) {
    Serial.println("Received id was same as my own id.  Ignoring.");
    return;
  }
  if (payload.command == RUNNING) {
    if (mode == DUST_COLLECTOR) {
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
      int myCode = currentGateCode();
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
    Payload send;
    send.id = id;
    send.gateCode = currentGateCode();
    send.command = WELCOME;
    memcpy(send.debugMessage, "Welcome", 7);
    broadcastCommand(send);
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
  if (USE_FAKE_CURRENT) {
    if (analogRead(CURRENT_SENSOR_PIN) > 512) {
      return false;
    }
    return true;
  }
  return currentAmps() >= MIN_CURRENT_TO_ACTIVATE;
}

