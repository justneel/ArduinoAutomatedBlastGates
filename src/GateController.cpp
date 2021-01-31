#include "GateController.h"
#include "Constants.h"
#include "GatePins.h"
#include <EEPROM.h>

const unsigned long ANALOG_READ_SAMPLE_DURATION_MS = 100;

int averageAnalogRead(int pin) {
    unsigned long numReads = 0;
    unsigned long total = 0;
    unsigned long endReadTime = millis() + ANALOG_READ_SAMPLE_DURATION_MS;
    while (millis() < endReadTime) {
        numReads++;
        total += analogRead(pin);
    }
    return total / numReads;
}

GateController::GateController() : openPotPin(OPEN_POT_PIN), closedPotPin(CLOSED_POT_PIN) {};

void GateController::setup() {
    if (mode == DUST_COLLECTOR) {
        return;
    }
    
    currentGateState = CLOSED;
    if (SERIAL_CALIBRATION) {
        EEPROM.get(0, gatePositions);
        Serial.print("Loaded gate positions from memory: open=");
        Serial.print(gatePositions.openPosition);
        Serial.print(" closed=");
        Serial.println(gatePositions.closedPosition);
    } else {
        pinMode(openPotPin, INPUT);
        pinMode(closedPotPin, INPUT);

        lastOpenPinAnalogReading = averageAnalogRead(openPotPin);
        lastClosedPinAnalogReading = averageAnalogRead(closedPotPin);

        currentServoPosition = lastClosedPinAnalogReading;
        servo.write(analogToServoPosition(currentServoPosition));
    }
    servo.attach(SERVO_PIN, 500, 2500);
}

CalibrateStatus GateController::calibrate(int pin, int& lastReadValue, bool& inCalibration) {
    int newReading = averageAnalogRead(pin);
    int diff = abs(newReading - lastReadValue);
    if ((!inCalibration && diff >= BEGIN_CALIBRATION_CHANGE_AMOUNT)
            || (inCalibration && diff >= IN_CALIBRATION_ANALOG_FLOAT_AMOUNT)) {
        if (!inCalibration) {
            Serial.print("Entering calibration for: ");
            if (pin == openPotPin) {
                Serial.println("open position");
            } else {
                Serial.println("close position");
            }
            inCalibration = true;
        }
        calibrationUpdateTime = millis();
        lastReadValue = newReading;
        
        int newServoPosition = analogToServoPosition(newReading);
        Serial.print("IN CALIBRATION - going to position: ");
        Serial.println(newServoPosition);
        
        goToPosition(newServoPosition);
        return IN_CALIBRATION;
    } else if (inCalibration && (calibrationUpdateTime + TIME_TO_CALIBRATE_MS) <= millis()) {
        inCalibration = false;
        return LEAVING_CALIBRATION;
    }
    return NO_CALIBRATION;
}

void GateController::onLoop() {
    if (mode == DUST_COLLECTOR) {
        return;
    }
    if (SERIAL_CALIBRATION) {
        calibrationUpdateTime = 0;
        inOpenCalibration = false;
        inCloseCalibration = false;
        bool positionsUpdated = false;
        while (Serial.available() || (inCalibration() && (calibrationUpdateTime + TIME_TO_CALIBRATE_MS) > millis())) {

            String input = Serial.readStringUntil(' ');
            int newPosition = Serial.parseInt();
            if (newPosition > 0 && newPosition <= 180) {
                calibrationUpdateTime = millis();

                if (input.startsWith("o")) {
                    Serial.print("Updating open position to: ");
                    Serial.println(newPosition);
                    inOpenCalibration = true;
                    positionsUpdated = positionsUpdated || gatePositions.openPosition != newPosition;
                    gatePositions.openPosition = newPosition;
                    calibrationUpdateTime = millis();
                    goToPosition(newPosition);
                
                } else if (input.startsWith("c")) {
                    Serial.print("Updating closed position to: ");
                    Serial.println(newPosition);
                    inCloseCalibration = true;
                    positionsUpdated = positionsUpdated || gatePositions.openPosition != newPosition;
                    gatePositions.closedPosition = newPosition;
                    calibrationUpdateTime = millis();
                    goToPosition(newPosition);
                }
            }   
        }
        
        if (inCalibration()) {
            Serial.println("Serial calibration complete");
            inOpenCalibration = false;
            inCloseCalibration = false;
            if (positionsUpdated) {
                Serial.print("Saving new gate positions: open=");
                Serial.print(gatePositions.openPosition);
                Serial.print(" closed=");
                Serial.println(gatePositions.closedPosition);
                EEPROM.put(0, gatePositions);
            }
            if (currentGateState == OPEN) {
                goToPosition(gatePositions.openPosition);
            } else if (currentGateState == CLOSED) {
                goToPosition(gatePositions.closedPosition);
            }
        }
        
    } else {
        CalibrateStatus status;
        bool calibrationDone = false;
        if (inOpenCalibration) {
            status = calibrate(openPotPin, lastOpenPinAnalogReading, inOpenCalibration);
            if (status == LEAVING_CALIBRATION) {
                Serial.println("Finished open calibration");
                calibrationDone = true;
            }
        } else if (inCloseCalibration) {
            status = calibrate(closedPotPin, lastClosedPinAnalogReading, inCloseCalibration);
            if (status == LEAVING_CALIBRATION) {
                Serial.println("Finished closed calibration");
                calibrationDone = true;
            }
        } else {
            // check both
            status = calibrate(openPotPin, lastOpenPinAnalogReading, inOpenCalibration);
            if (status != IN_CALIBRATION) {
                calibrate(closedPotPin, lastClosedPinAnalogReading, inCloseCalibration);
            }
        }

        if (calibrationDone) {
            if (currentGateState == OPEN) {
                goToAnalogPosition(lastOpenPinAnalogReading);
            } else {
                goToAnalogPosition(lastClosedPinAnalogReading);
            }
        }
    }
}

bool GateController::isOpen() {
    return currentGateState == OPEN;
}

void GateController::openGate() {
    if (currentGateState != OPEN) {
        currentGateState = OPEN;
        if (!inCalibration()) {
            Serial.println("Opening the gate");
            goToAnalogPosition(lastOpenPinAnalogReading);
        } else {
            Serial.println("Open gate requested, but currently in calibration mode.  Ignoring");
        }
    }
}

bool GateController::isClosed() {
    return currentGateState == CLOSED;
}

void GateController::closeGate() {
    if (currentGateState != CLOSED) {
        currentGateState = CLOSED;
        if (!inCalibration()) {
            Serial.println("Closing the gate");
            goToAnalogPosition(lastClosedPinAnalogReading);
        } else {
            Serial.println("Close gate requested, but currently in calibration mode.  Ignoring");
        }
    }
}

int GateController::analogToServoPosition(int analogValue) {
    // return round(((double) analogValue / ANLOG_MAX_VALUE) * MAX_ROTATION);
    return map(analogValue, 0, 1023, 0, MAX_ROTATION);
}

void GateController::goToAnalogPosition(int analogValue) {
    goToPosition(analogToServoPosition(analogValue));
}

void GateController::goToPosition(const int position) {
    if (position == currentServoPosition) {
        // Serial.println("Already at requested position.  Not moving");
        return;
    }

    // Serial.print("Target position: ");
    // Serial.println(position);

    int inc;
    if (currentServoPosition > position) {
        inc = -1;
    } else {
        inc = 1;
    }

    do {
        currentServoPosition += inc;
        servo.write(currentServoPosition);
        delay(DELAY_BETWEEN_SERVO_STEPS_MS);
    } while (position != currentServoPosition);
}