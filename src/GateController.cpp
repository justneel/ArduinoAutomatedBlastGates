#include "GateController.h"

const unsigned long TIME_TO_CALIBRATE_MS = 10000;
const double ANLOG_MAX_VALUE = 1023;
const double MAX_ROTATION = 270;

const int ANALOG_FLOAT_AMOUNT = 2;

GateController::GateController() : openPotPin(OPEN_POT_PIN), closedPotPin(CLOSED_POT_PIN) {
    pinMode(openPotPin, INPUT);
    pinMode(closedPotPin, INPUT);
    pinMode(SERVO_POWER_PIN, OUTPUT);

    myservo.attach(SERVO_PIN, 500, 2500);

    // TODO: Save and load from eeprom
    lastOpenPinAnalogReading = analogRead(openPotPin);
    lastClosedPinAnalogReading = analogRead(closedPotPin);

    currentGateState = CLOSED;
    currentServoPosition = analogToServoPosition(lastClosedPinAnalogReading);
};

void GateController::onLoop() {
    int openPosition = analogRead(openPotPin);
    int closedPosition = analogRead(closedPotPin);
    bool openChanged = abs(openPosition - lastOpenPinAnalogReading) > ANALOG_FLOAT_AMOUNT;
    bool closedChanged = abs(closedPosition - lastClosedPinAnalogReading) > ANALOG_FLOAT_AMOUNT;
    // Serial.println("onloop");
    if (openChanged || closedChanged) {
        Serial.println("In Calibration of gate");
        inCalibration = true;
        calibrationUpdateTime = millis();
        if (openChanged) {
            Serial.print("Changing open position from ");
            Serial.print(lastOpenPinAnalogReading);
            Serial.print(" to ");
            Serial.println(openPosition);
            lastOpenPinAnalogReading = openPosition;
            goToAnalogPosition(openPosition);
        } else if (closedChanged) {
            Serial.print("Changing closed position from ");
            Serial.print(lastClosedPinAnalogReading);
            Serial.print(" to ");
            Serial.println(closedPosition);
            lastClosedPinAnalogReading = closedPosition;
            goToAnalogPosition(closedPosition);
        }
    } else if (inCalibration && (calibrationUpdateTime + TIME_TO_CALIBRATE_MS) < millis()) {
        Serial.println("Calibration over.  Returning to actual positions");
        inCalibration = false;
        if (currentGateState == OPEN) {
            goToAnalogPosition(openPosition);
        } else {
            goToAnalogPosition(closedPosition);
        }
    }
}

void GateController::openGate() {
    if (currentGateState != OPEN) {
        currentGateState = OPEN;
        Serial.println("Opening the gate");
        if (!inCalibration) {
            goToAnalogPosition(lastOpenPinAnalogReading);
        }
    }
}

void GateController::closeGate() {
    if (currentGateState != CLOSED) {
        currentGateState = CLOSED;
        Serial.println("Closing the gate");
        if (!inCalibration) {
            goToAnalogPosition(lastClosedPinAnalogReading);
        }
    }
}

int GateController::analogToServoPosition(int analogValue) {
    return round(((double) analogValue / ANLOG_MAX_VALUE) * MAX_ROTATION);
}

void GateController::goToAnalogPosition(int analogValue) {
    goToPosition(analogToServoPosition(analogValue));
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