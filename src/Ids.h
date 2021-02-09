#ifndef ids_h
#define ids_h

#include "Constants.h"

class Ids {
    public:
        void setup();
        unsigned int currentGateCode();
        unsigned long getID() const { return id; }
        void populateId();
    private:
        unsigned long id = VALUE_UNSET;
};

#endif