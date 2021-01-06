#ifndef gate_controller_h
#define gate_controller_h

#include <Arduino.h>
#include <Servo.h>
#include "GatePins.h"

enum GateState {
  OPEN,
  CLOSED
};

class GateController {
    public:
        GateController();
        void onLoop();
        void openGate();
        void closeGate();
    private:
        GateState currentGateState;
        bool inCalibration = false;
        unsigned long calibrationUpdateTime = 0;
        const int openPotPin;
        const int closedPotPin;

        int lastOpenPositionReading;
        int lastClosedPositionReading;

        int currentServoPosition;
        Servo myservo;

        int getPositionForServo(int pin);
        void goToPosition(int position);
};

#endif