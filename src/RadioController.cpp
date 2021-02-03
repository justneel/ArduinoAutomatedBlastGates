#include "RadioController.h"
#include <limits.h>

void printId(unsigned long id) {
  if (id == VALUE_UNSET) {
    Serial.print("UNSET");
  } else {
    Serial.print(id);
  }
}

void RadioController::print(const Payload &payload) {
  Serial.print("Payload {");
  Serial.print(" messageId=");
  Serial.print(payload.messageId);
  Serial.print(" id=");
  printId(payload.id);
  Serial.print(" toId=");
  printId(payload.toId);
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
    case ACK:
      Serial.print("ACK");
      break;
    default:
      Serial.print("UNKNOWN");
  }
  // Serial.print(" debug=");
  // Serial.print(payload.debugMessage);
  Serial.print(" ");
  Serial.print(" }");
}

void RadioController::println(const Payload &payload) {
  print(payload);
  Serial.println(" ");
}

void RadioController::setup() {
    // Let the capacitor charge up before turning on the radio.
  // No idea if this will actually do anything.
  // delay(100);
    configureRadioForNormalReading(); 

    if (mode == DUST_COLLECTOR) {
        id = DUST_COLLECTOR_ID;
    } else {
        randomSeed(analogRead(A0));

        for (int i = 0; i < BRANCH_PINS_LENGTH; i++) {
            pinMode(BRANCH_PINS[i], INPUT_PULLUP);
        }
    }
}

void RadioController::onLoop() {
    if (radioFailed()) {
        Serial.println("Radio failure detected.");
        radio.failureDetected = true;
        configureRadioForNormalReading();
    }
}

void RadioController::configureRadio() {
  radio.failureDetected = false;
  while (!radio.begin() || !radio.isChipConnected()) {
    Serial.print(millis() / 1000);
    Serial.println(" Waiting for radio to start");
    // radio.printDetails();
    delay(50);
  }
  radio.setPALevel(RADIO_POWER_LEVEL);
  radio.setChannel(CHANNEL);
  radio.setAutoAck(false);
  if (!radio.setDataRate(RADIO_DATA_RATE)) {
    Serial.println("Could not set the data rate");
    radio.failureDetected = true;
  }
  radio.setPayloadSize(payloadSize);
  radio.setCRCLength(RF24_CRC_8);

  radio.openReadingPipe(BROADCAST_PIPE, myAddress);
}

void RadioController::configureRadioForNormalReading() {
  configureRadio();

  radio.startListening();
}

void RadioController::configureRadioForAckReading() {
  configureRadio();
  radio.openReadingPipe(ACK_PIPE, ackAddress);
  radio.startListening();
}

bool RadioController::radioFailed() {
  if (radio.failureDetected 
      || radio.getDataRate() != RADIO_DATA_RATE 
      || radio.getPALevel() != RADIO_POWER_LEVEL) {
    radio.failureDetected = true;
    return true;
  }
  return false;
}

unsigned long RadioController::getNextMessageId() {
    unsigned long messageId = ++currentMessageId;
    if (currentMessageId >= (LONG_MAX - 1)) {
        currentMessageId = 1;
    }
    return messageId;
}

bool RadioController::getMessage(Payload &received) {
    uint8_t incomingPipe;
    if (radio.available(&incomingPipe) && incomingPipe == BROADCAST_PIPE) {
        radio.read(&received, payloadSize);

        if (received.messageId == 0 || received.command == UNKNOWN) {
            // Received a blank message.  Just ignore.
            return false;
        }
        
        Serial.print(millis() / 1000.0);
        Serial.print(" Received: ");
        println(received);

        if (received.id == id && received.id != VALUE_UNSET) {
            Serial.println("Received id was same as my own id.  Ignoring.");
            return false;
        }
        if (received.toId != VALUE_UNSET && received.toId != id) {
            Serial.print("Message was directed to another id: ");
            printId(received.id);
            Serial.print(" vs my id: ");
            printId(id);
            Serial.println(" Ignoring command");
            return false;
        }
        return true;
  }
  return false;
}

bool RadioController::sendAck(const Payload &originalMessage) {
    Payload ackMessage;
    ackMessage.messageId = getNextMessageId();
    ackMessage.id = id;
    ackMessage.toId = originalMessage.id;
    ackMessage.command = ACK;
    return broadcastCommand(ackMessage, false);
}

bool RadioController::broadcastCommand(Command command) {
    return broadcastCommand(command, false);
}

bool RadioController::broadcastCommand(Command command, boolean ack) {
    if (command == RUNNING && id == VALUE_UNSET) {
        // If the first run is not from when we just started, we can use the
        // millis() as a good random seed.
        if (millis() > 10000) {
            randomSeed(millis());
        }
        id = abs(random(2147483600));
    }

    Payload sendPayload;
    sendPayload.messageId = getNextMessageId();
    sendPayload.command = command;
    sendPayload.id = id;
    sendPayload.gateCode = currentGateCode();

    return broadcastCommand(sendPayload, ack);   
}

bool RadioController::broadcastCommand(const Payload &payload, boolean ack) {
  
  Serial.print(millis() / 1000.0);
  Serial.print(" Broadcasting: ");
  println(payload);

  unsigned long messageSentTime = millis();

  while (radioFailed()) {
    Serial.println("Radio failed while trying to send out message.");
    configureRadioForNormalReading();
  }

  radio.stopListening();
  if (payload.command == ACK) {
    radio.openWritingPipe(ackAddress);
  } else {
    radio.openWritingPipe(sendAddress);
  }
  radio.write(&payload, payloadSize);
  // delayMicroseconds(250);
  if (ack) {
    radio.openReadingPipe(ACK_PIPE, ackAddress);
  }
  radio.startListening();
  if (ack) {
    boolean ackReceived = false;
    int retries = 0;
    Payload incomingMessage;
    while (!ackReceived && retries < BROADCAST_RETRIES) {
      if (radioFailed()) {
        Serial.println("radio failed while in loop");
        configureRadioForAckReading();
        radio.openReadingPipe(ACK_PIPE, ackAddress);
        radio.startListening();
      }
      if (radio.available()) {
        radio.read(&incomingMessage, payloadSize);
        if (incomingMessage.command == ACK && incomingMessage.toId == id) {
          // Serial.println("ACK received");
          ackReceived = true;
        } else {
          Serial.print("Waiting for ack, but another message was received instead: ");
          println(incomingMessage);
        }
      } else if ((messageSentTime + BROADCAST_RETRY_DELAY) < millis()) {
        radio.stopListening();
        radio.openWritingPipe(sendAddress);
        radio.write(&payload, payloadSize);
        messageSentTime = millis();
        retries++;
        radio.startListening();
      }
    }
    if (ackReceived) {
      Serial.print("Ack received after retries: ");
      Serial.println(retries);
    }
    if (!ackReceived) {
      Serial.println("Never received ack for command");
    }

    radio.closeReadingPipe(ACK_PIPE);
    radio.startListening();
    return ackReceived;
  }
  return true;
}

unsigned int RadioController::currentGateCode() {
    if (mode == DUST_COLLECTOR) {
        return 0;
    }
    unsigned int value = 0;
    for (int i = 0; i < BRANCH_PINS_LENGTH; i++) {
        value *= 2;
        if (digitalRead(BRANCH_PINS[i]) == LOW) {
        value++;
        }
    }
    return value;
}