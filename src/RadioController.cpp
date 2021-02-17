#include "RadioController.h"
#include <limits.h>
#include "Log.h"

void RadioController::setup() {
    replyToAcks = mode == DUST_COLLECTOR;
    configureRadio();
}

void RadioController::onLoop() {
    if (radioFailed()) {
        Serial.println("Radio failure detected.");
        radio.failureDetected = true;
        configureRadio();
    }
}

void RadioController::configureRadio() {
  radio.failureDetected = false;
  while (!radio.begin() || !radio.isChipConnected()) {
    statusController.setRadioInFailure(true);
    // TODO: failure LED
    Serial.print(millis() / 1000);
    Serial.println(" Waiting for radio to start");
    // radio.printDetails();
    delay(500);
  }
  radio.setPALevel(RADIO_POWER_LEVEL);
  radio.setChannel(CHANNEL);
  radio.setAutoAck(false);

  if (!radio.setDataRate(RADIO_DATA_RATE)) {
    Serial.println("Could not set the data rate");
    radio.failureDetected = true;
    return;
  }
  radio.setPayloadSize(payloadSize);
  
  radio.setCRCLength(RF24_CRC_8);
  // radio.flush_rx();
  // radio.flush_tx();

  radio.openReadingPipe(BROADCAST_PIPE, myAddress);
  if (mode != DUST_COLLECTOR && !USE_CHIP_ACK) {
    radio.openReadingPipe(ACK_PIPE, ackAddress);
  }

  if (USE_CHIP_ACK) {
    radio.setAutoAck(true);
    // radio.enableDynamicAck();
    radio.enableDynamicPayloads();
    dynamicPayloadsEnabled = true;

    if (mode == DUST_COLLECTOR) {
      // radio.setAutoAck(BROADCAST_PIPE, true);
      // radio.enableAckPayload();

      // Payload ackPayload;
      // ackPayload.id = id;
      // ackPayload.command = ACK;
      // radio.writeAckPayload(BROADCAST_PIPE, &ackPayload, payloadSize);
    } else {
      // radio.setAutoAck(BROADCAST_PIPE, false);
      // radio.setAutoAck(0, true);
    }
  }

  radio.startListening();
  statusController.setRadioInFailure(false);
  // Serial.println("------------ After Configure -----------");
  // radio.printPrettyDetails();
  // Serial.println("----------------------------------------");
}

bool RadioController::radioFailed() {
  if (radio.failureDetected 
      || radio.getDataRate() != RADIO_DATA_RATE 
      || radio.getPALevel() != RADIO_POWER_LEVEL) {
        if (radio.failureDetected) {
          Serial.print("Failure from failureDetected.   ");
        } else if (radio.getDataRate() != RADIO_DATA_RATE) {
          Serial.print("Failure from data rate change.   ");
        } else if (radio.getPALevel() != RADIO_POWER_LEVEL) {
          Serial.print("Failure from power level.   ");
        } else {
          Serial.print("Failure from unknown.   ");
        }
    // Serial.println("-------------- After Failure -----------");
    // radio.printPrettyDetails();
    // Serial.println("----------------------------------------");
    radio.failureDetected = true;
    statusController.setRadioInFailure(true);
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
    if (radio.available(&incomingPipe)) {
        radio.read(&received, (dynamicPayloadsEnabled) ? radio.getDynamicPayloadSize() : radio.getPayloadSize());

        if (received.messageId == 0 || received.command == UNKNOWN) {
            // Received a blank message.  Just ignore.
            Serial.println("Received blank message");
            return false;
        }
        
        Serial.print(millis() / 1000.0);
        Serial.print(" Received: ");
        println(received);
        maybeAck(received);

        unsigned long myId = ids.getID();
        if (received.id == myId && myId != VALUE_UNSET) {
            Serial.println("Received id was same as my own id.  Ignoring.");
            return false;
        }
        if (received.toId != VALUE_UNSET && received.toId != ids.getID()) {
            Serial.print("Message was directed to another id: ");
            printId(received.id);
            Serial.print(" vs my id: ");
            printId(myId);
            Serial.println(" Ignoring command");
            return false;
        }
        return true;
  }
  return false;
}

bool RadioController::broadcastCommand(Command command) {
    return broadcastCommand(command, false);
}

bool RadioController::broadcastCommand(Command command, boolean ack) {
    if (command == RUNNING) {
      ids.populateId();
    }

    Payload sendPayload;
    sendPayload.messageId = getNextMessageId();
    sendPayload.command = command;
    sendPayload.id = ids.getID();
    sendPayload.gateCode = ids.currentGateCode();
    sendPayload.requestACK = ack;

    return broadcastCommand(sendPayload);   
}

bool RadioController::broadcastCommand(const Payload &payload) {
  
  Serial.print(millis() / 1000.0);
  Serial.print(" Broadcasting: ");
  println(payload);

  if (USE_CHIP_ACK) {
    radio.stopListening();
    radio.openWritingPipe(sendAddress);
    bool requestAck = payload.requestACK;
    bool success = radio.write(&payload, payloadSize, !requestAck);
    // success = radio.txStandBy(1000) || success;
    if (requestAck) {
      if (success) {
        Serial.println("Message sucessfully sent");
      } else {
        Serial.println("Failed to send message");
      }
      statusController.setTransmissionStatus(success);
    }
    radio.startListening();
    return success;
  }

  boolean received = !payload.requestACK;
  // boolean received = true;
  int retries = 0;
  do {
    while (radioFailed() && retries < BROADCAST_RETRIES) {
      Serial.println("Radio failed while trying to send out message.");
      retries++;
      configureRadio();
    }
    if (retries >= BROADCAST_RETRIES) {
      Serial.println("Could not configure radio.  Dropping message");
      return false;
    }
    radio.stopListening();
    if (payload.command == ACK) {
      radio.openWritingPipe(ackAddress);
    } else {
      radio.openWritingPipe(sendAddress);
    }
    radio.writeFast(&payload, payloadSize);
    if (!radio.txStandBy(500)) {
      Serial.println("txStandby failed");
    }
    retries++;
    radio.startListening();
    if (payload.requestACK) {
      received = waitForAckPayload(BROADCAST_RETRY_DELAY_MS);
    }
  } while (!received && retries < BROADCAST_RETRIES);

  if (payload.requestACK) {
    if (received) {
      Serial.print("Message sent sucessfully after ");
      Serial.print(retries - 1);
      Serial.println(" retries");
    } else {
      Serial.println("Message failed");
    }
    statusController.setTransmissionStatus(received);
  }
  return received;
}


void RadioController::maybeAck(const Payload &received) {
  if (!USE_CHIP_ACK && replyToAcks && received.requestACK) {
    Payload ackPayload;
    ackPayload.messageId = getNextMessageId();
    ackPayload.id = ids.getID();
    ackPayload.toId = received.id;
    ackPayload.command = ACK;
    broadcastCommand(ackPayload);
  }
}

bool RadioController::waitForAckPayload(unsigned long maxWait) {
  // unsigned long startWait = millis();
  unsigned long endWaitTime = millis() + maxWait;
  Payload received;
  uint8_t incomingPipe;
  while (millis() < endWaitTime) {
    while (radioFailed() && millis() < endWaitTime) {
      Serial.println("Radio failed while waitin for ACK.");
      configureRadio();
    }
    if (radio.available(&incomingPipe)) {
      radio.read(&received, (dynamicPayloadsEnabled) ? radio.getDynamicPayloadSize() : radio.getPayloadSize());
      if (received.command == ACK && received.toId == ids.getID()) {
        return true;
      } else {
        Serial.print("Receieved an unexpected message while waiting for ack: ");
        println(received);
      }
    } else {
      // Serial.println("Nothing available from the radio yet");
    }
  }
  return false;
}

