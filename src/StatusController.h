#ifndef status_controller_h
#define status_controller_h

const unsigned long FAILED_TRANSMISSION_LIGHT_ON_TIME_MS = 10000;
const unsigned long CALIBRATION_BLINK_MS = 300;

class StatusController {
    public:
        void setup();
        void onLoop();
        void setTransmissionStatus(bool success);
        void setGateStatus(bool open);
        void setCalibrationMode(bool inCalibration);
    private:
        unsigned long lastFailedTranmissionTime = 0;
        bool inCalibrationMode = false;
        unsigned long lastCalibrationBlinkChange = 0;
        bool calibrationBlinkOn = false;
};

#endif