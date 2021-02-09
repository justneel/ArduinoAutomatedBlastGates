#ifndef radio_controller_h
#define radio_controller_h

#include <Arduino.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "Constants.h"
#include "GatePins.h"

const rf24_datarate_e RADIO_DATA_RATE = RF24_1MBPS;
const rf24_pa_dbm_e RADIO_POWER_LEVEL = RF24_PA_LOW;

const bool USE_CHIP_ACK = true;
// const bool SEPARATE_PIPE_FOR_ACK = true;

const unsigned long BROADCAST_RESPONSE_DELAY_MS = 10;
const unsigned long BROADCAST_RETRY_DELAY_MS = 10;
const int BROADCAST_RETRIES = 10;

const uint8_t myAddress =  0xDE;
const uint8_t sendAddress = myAddress;
const uint8_t ackAddress = 0xBE;

const uint8_t CHANNEL = 3;
// const uint8_t CHANNEL = 124;


const uint8_t BROADCAST_PIPE = 1;
const uint8_t ACK_PIPE = 2;

enum Command {
    UNKNOWN,
    RUNNING,
    NO_LONGER_RUNNING,
    ACK,
    HELLO_WORLD, // Debugging message sent out when a machine first comes online
    WELCOME, // Response back from the HELLO_WORLD
};

struct Payload {
  unsigned long messageId = VALUE_UNSET;
  unsigned long id = VALUE_UNSET;
  unsigned long toId = VALUE_UNSET;
  unsigned int gateCode = 0;
  Command command = UNKNOWN;
  boolean requestACK = false;
};

const int payloadSize = sizeof(Payload);

class RadioController {
    public:
        RadioController() {};
        void setup();
        void onLoop();
        void configureRadio();
        bool radioFailed();
        bool broadcastCommand(Command command);
        bool broadcastCommand(Command command, boolean ack);
        bool getMessage(Payload &buff);

        unsigned int currentGateCode();

        // void print(const Payload &payload);
        // void println(const Payload &payload);
    private:
        RF24 radio = RF24(CE_PIN, CSN_PIN);
        unsigned long currentMessageId = 0;
        unsigned long id = VALUE_UNSET;
        boolean replyToAcks = false;
        void maybeAck(const Payload &received);
        bool waitForAckPayload(unsigned long maxWait);
        bool broadcastCommand(const Payload &payload);
        unsigned long getNextMessageId();
        bool dynamicPayloadsEnabled = false;
};

#endif