#ifndef gate_controller_h
#define gate_controller_h

#include <Arduino.h>
#include <Servo.h>
#include "StatusController.h"
#include "Ids.h"

enum GateState {
  OPEN,
  CLOSED
};

enum CalibrateStatus {
    NO_CALIBRATION,
    IN_CALIBRATION,
    LEAVING_CALIBRATION,
};


struct GatePositions {
    int closedPosition = 0;
    int openPosition = 180;
};

class GateController {
    public:
        GateController(StatusController &sc, Ids &ids)  : statusController(sc), ids(ids) {};
        void setup();
        void onLoop();
        void openGate();
        void closeGate();
        bool isClosed();
        bool isOpen();
    private:
        StatusController &statusController;
        Ids &ids;
        
        GateState currentGateState;
        GatePositions gatePositions;

        bool inOpenCalibration = false;
        bool inCloseCalibration = false;
        unsigned long calibrationUpdateTime = 0;
        

        // int lastOpenPositionReading;
        // int lastClosedPositionReading;
        int lastOpenPinAnalogReading;
        int lastClosedPinAnalogReading;

        int currentServoPosition;
        Servo servo;

        int analogToServoPosition(int analogValue);
        void goToAnalogPosition(int analogValue);
        void goToPosition(int position);
        CalibrateStatus calibrate(int pin, int& lastReadValue, bool& inCalibration);
        
        bool inCalibration() { return inOpenCalibration || inCloseCalibration; }
};

#endif