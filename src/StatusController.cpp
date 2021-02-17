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

    radioFailureBlinker.setup();
    calibrationBlinker.setup();
}

void StatusController::onLoop() {
    radioFailureBlinker.onLoop();
    calibrationBlinker.onLoop();

    if (!radioFailureBlinker.isEnabled()) {
        if (lastFailedTranmissionTime + FAILED_TRANSMISSION_LIGHT_ON_TIME_MS < millis()) {
            digitalWrite(RED_LED, LOW);
        } else {
            digitalWrite(RED_LED, HIGH);
        }
    }
    if (!calibrationBlinker.isEnabled()) {
        digitalWrite(BLUE_LED, gateStatus ? HIGH : LOW);
    }

    if ((SYSTEM_ACTIVE_MS + lastActiveTime) > millis()) {
        digitalWrite(GREEN_LED, HIGH);
    } else {
        digitalWrite(GREEN_LED, LOW);
    }
}

void StatusController::onSystemActive() {
    lastActiveTime = millis();
}

void StatusController::setRadioInFailure(bool inFailure) {
    radioFailureBlinker.setEnabled(inFailure);
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
    gateStatus = open;
}

void StatusController::setCalibrationMode(bool inCalibration) {
    calibrationBlinker.setEnabled(inCalibration);
}