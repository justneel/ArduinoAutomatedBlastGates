#ifndef radio_controller_h
#define radio_controller_h

#include <Arduino.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "Constants.h"
#include "GatePins.h"

const rf24_datarate_e RADIO_DATA_RATE = RF24_250KBPS;
const rf24_pa_dbm_e RADIO_POWER_LEVEL = RF24_PA_HIGH;

const int BROADCAST_RETRIES = 50;
const unsigned long BROADCAST_RETRY_DELAY = 50;

const uint8_t BROADCAST_PIPE = 1;
const uint8_t ACK_PIPE = 2;

enum Command {
    UNKNOWN,
    ACK,
    RUNNING,
    NO_LONGER_RUNNING,
    HELLO_WORLD, // Debugging message sent out when a machine first comes online
    WELCOME, // Response back from the HELLO_WORLD
};

struct Payload {
  unsigned long messageId = VALUE_UNSET;
  unsigned long id = VALUE_UNSET;
  unsigned long toId = VALUE_UNSET;
  unsigned int gateCode = 0;
  Command command = UNKNOWN;
};

const int payloadSize = sizeof(Payload);

class RadioController {
    public:
        RadioController() {};
        void setup();
        void onLoop();
        void configureRadio();
        void configureRadioForNormalReading();
        void configureRadioForAckReading();
        bool radioFailed();
        bool broadcastCommand(Command command);
        bool broadcastCommand(Command command, boolean ack);
        bool sendAck(const Payload &originalMessage);
        bool getMessage(Payload &buff);

        unsigned int currentGateCode();

        void print(const Payload &payload);
        void println(const Payload &payload);
    private:
        RF24 radio = RF24(CE_PIN, CSN_PIN);
        unsigned long currentMessageId = 0;
        unsigned long id = VALUE_UNSET;

        bool broadcastCommand(const Payload &payload, boolean ack);
        unsigned long getNextMessageId();
};

#endif