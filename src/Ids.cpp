#include "Ids.h"
#include "Constants.h"
#include "GatePins.h"

void Ids::setup() {
    if (mode == DUST_COLLECTOR) {
        id = DUST_COLLECTOR_ID;
    } else {
        // randomSeed(analogRead(A0));

        for (int i = 0; i < BRANCH_PINS_LENGTH; i++) {
            pinMode(BRANCH_PINS[i], INPUT_PULLUP);
        }
    }
}

unsigned int Ids::currentGateCode() {
    if (mode == DUST_COLLECTOR) {
        return 0;
    }
    unsigned int value = 0;
    for (int i = 0; i < BRANCH_PINS_LENGTH; i++) {
        value *= 2;
        if (digitalRead(BRANCH_PINS[i]) == LOW) {
        value++;
        }
    }
    return value;
}

void Ids::populateId() {
    if (id == VALUE_UNSET) {
        // If the first run is not from when we just started, we can use the
        // millis() as a good random seed.
        if (millis() > 10000) {
            randomSeed(millis());
        }
        id = abs(random(2147483600));
    }
}
