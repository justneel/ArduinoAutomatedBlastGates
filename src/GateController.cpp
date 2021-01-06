#include "GateController.h"

const unsigned long TIME_TO_CALIBRATE_MS = 10000;
const double ANLOG_MAX_VALUE = 1023;
const double MAX_ROTATION = 270;

GateController::GateController() : openPotPin(OPEN_POT_PIN), closedPotPin(CLOSED_POT_PIN) {
    pinMode(openPotPin, INPUT);
    pinMode(closedPotPin, INPUT);
    pinMode(SERVO_POWER_PIN, OUTPUT);

    myservo.attach(SERVO_PIN, 500, 2500);

    // TODO: Save and load from eeprom
    lastOpenPositionReading = getPositionForServo(openPotPin);
    lastClosedPositionReading = getPositionForServo(closedPotPin);

    currentGateState = CLOSED;
    currentServoPosition = lastClosedPositionReading;
};

void GateController::onLoop() {
    // int openPosition = getPositionForServo(openPotPin);
    // int closedPosition = getPositionForServo(closedPotPin);
    // bool openChanged = abs(openPosition - lastOpenPositionReading) > 2;
    // bool closedChanged = abs(closedPosition - lastClosedPositionReading) > 2;
    // // Serial.println("onloop");
    // if (openChanged || closedChanged) {
    //     Serial.println("In Calibration of gate");
    //     inCalibration = true;
    //     calibrationUpdateTime = millis();
    //     if (openChanged) {
    //         Serial.print("Changing open position from ");
    //         Serial.print(lastOpenPositionReading);
    //         Serial.print(" to ");
    //         Serial.println(openPosition);
    //         lastOpenPositionReading = openPosition;
    //         goToPosition(openPosition);
    //     } else if (closedChanged) {
    //         Serial.print("Changing closed position from ");
    //         Serial.print(lastClosedPositionReading);
    //         Serial.print(" to ");
    //         Serial.println(closedPosition);
    //         lastClosedPositionReading = closedPosition;
    //         goToPosition(closedPosition);
    //     }
    // } else if (inCalibration && (calibrationUpdateTime + TIME_TO_CALIBRATE_MS) < millis()) {
    //     Serial.println("Calibration over.  Returning to actual positions");
    //     inCalibration = false;
    //     if (currentGateState == OPEN) {
    //         goToPosition(openPosition);
    //     } else {
    //         goToPosition(closedPosition);
    //     }
    // }
}

void GateController::openGate() {
    if (currentGateState != OPEN) {
        currentGateState = OPEN;
        Serial.println("Opening the gate");
        if (!inCalibration) {
            goToPosition(lastOpenPositionReading);
        }
    }
}

void GateController::closeGate() {
    if (currentGateState != CLOSED) {
        currentGateState = CLOSED;
        Serial.println("Closing the gate");
        if (!inCalibration) {
            goToPosition(lastClosedPositionReading);
        }
    }
}

int GateController::getPositionForServo(int pin) {
    return ((double) analogRead(pin) / ANLOG_MAX_VALUE) * MAX_ROTATION;
}

void GateController::goToPosition(int position) {
    if (position == currentServoPosition) {
        return;
    }

    // Serial.print("Target position: ");
    // Serial.println(position);
    analogWrite(SERVO_POWER_PIN, HIGH);
    delay(30);

    int inc;
    if (currentServoPosition > position) {
        inc = -1;
    } else {
        inc = 1;
    }
    // Serial.print("inc: " );
    // Serial.println(incPerMs);

    int i = 0;
    do {
        currentServoPosition += inc;
        myservo.write(currentServoPosition);
        delay(10);
        i++;
    } while (position != currentServoPosition && i < MAX_ROTATION);

    analogWrite(SERVO_POWER_PIN, LOW);
    // Serial.println("Position achieved");
}