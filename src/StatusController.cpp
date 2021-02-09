#include "StatusController.h"
#include <Arduino.h>
#include "GatePins.h"

void StatusController::setup() {
    pinMode(RED_LED, OUTPUT);
    pinMode(BLUE_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);

    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
}

void StatusController::onLoop() {
    if (lastFailedTranmissionTime + FAILED_TRANSMISSION_LIGHT_ON_TIME_MS < millis()) {
        digitalWrite(RED_LED, LOW);
    }
    if (inCalibrationMode) {
        if ((lastCalibrationBlinkChange + CALIBRATION_BLINK_MS) < millis()) {
            lastCalibrationBlinkChange = millis();
            digitalWrite(GREEN_LED, (calibrationBlinkOn) ? LOW : HIGH);
            calibrationBlinkOn = !calibrationBlinkOn;
        }
    } else {
        digitalWrite(GREEN_LED, HIGH);
    }
}

void StatusController::setTransmissionStatus(bool success) {
    if (!success) {
        lastFailedTranmissionTime = millis();
        digitalWrite(RED_LED, HIGH);
    } else {
        digitalWrite(RED_LED, LOW);
        lastFailedTranmissionTime = 0;
    }
}

void StatusController::setGateStatus(bool open) {
    digitalWrite(BLUE_LED, open ? HIGH : LOW);
}

void StatusController::setCalibrationMode(bool inCalibration) {
    this->inCalibrationMode = inCalibration;
}