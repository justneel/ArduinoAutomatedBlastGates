#ifndef blinker_h
#define blinker_h

#include <Arduino.h>

class Blinker {
    public:
        Blinker(const int pin, const unsigned long blinkDelay) : pin(pin), blinkDelay(blinkDelay) {};
        void setup();
        void onLoop();
        bool isEnabled();
        void setEnabled(bool enabled);
    private:
        const int pin;
        const unsigned long blinkDelay;
        unsigned long lastBlinkMs = 0;
        bool ledOn = false;
        bool enabled = false;
};

#endif