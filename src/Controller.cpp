#include "Controller.h"

Controller::Controller(){}

void Controller::pressButton(int button){
    state &= ~(1 << button);
    requestInterrupt = true;
}

void Controller::releaseButton(int button){
    state |= (1 << button);
}

void Controller::write(uint8_t value){
    // The game only writes to bits 4 and 5 to select the matrix column
    selectBits = value & 0x30;
}

uint8_t Controller::read() const{
    uint8_t result = 0xCF; // Upper bits often return 1

    // If Action Buttons are selected (Bit 5 is 0)
    if ((selectBits & 0x20) == 0){
        result &= ((state >> 4) | 0xF0);
    }

    // If Direction Buttons are selected (Bit 4 is 0)
    else if ((selectBits & 0x10) == 0){
        result &= (state & 0x0F) | 0xF0;
    }

    return result;
}

bool Controller::checkInterrupt(){
    if (requestInterrupt){
        requestInterrupt = false;
        return true; // The CPU should trigger Interrupt Bit 4
    }

    return false;
}