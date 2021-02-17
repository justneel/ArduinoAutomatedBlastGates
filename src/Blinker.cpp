#include "Blinker.h"

void Blinker::setup() {
    pinMode(pin, OUTPUT);
};

void Blinker::onLoop() {
    if (enabled) {
        if ((lastBlinkMs + blinkDelay) < millis()) {
            lastBlinkMs = millis();
            ledOn = !ledOn;
            if (ledOn) {
                digitalWrite(pin, HIGH);
            } else {
                digitalWrite(pin, LOW);
            }
        }
    } else {
        digitalWrite(pin, LOW);
    }
};

void Blinker::setEnabled(bool enabled) {
    this->enabled = enabled;
}

bool Blinker::isEnabled() {
    return enabled;
}