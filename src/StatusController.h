#ifndef status_controller_h
#define status_controller_h

#include "Blinker.h"
#include "GatePins.h"

const unsigned long FAILED_TRANSMISSION_LIGHT_ON_TIME_MS = 10000;
const unsigned long CALIBRATION_BLINK_MS = 300;

const unsigned long RADIO_FAILURE_BLINK_MS = 300;

const unsigned long SYSTEM_ACTIVE_MS = 30L * 60L * 1000L;

class StatusController {
    public:
        StatusController() : radioFailureBlinker(RED_LED, RADIO_FAILURE_BLINK_MS), calibrationBlinker(BLUE_LED, CALIBRATION_BLINK_MS) {};
        void setup();
        void onLoop();
        void onSystemActive();
        void setTransmissionStatus(bool success);
        void setRadioInFailure(bool inFailure);
        void setGateStatus(bool open);
        void setCalibrationMode(bool inCalibration);
    private:
        unsigned long lastActiveTime = 0;
        unsigned long lastFailedTranmissionTime = 0;
        Blinker radioFailureBlinker;
        Blinker calibrationBlinker;
};

#endif