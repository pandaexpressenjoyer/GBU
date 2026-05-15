#include "Controller.h"

Controller::Controller() {
    // Start with all 8 buttons unpressed (0x0F = 0000 1111)
    actionButtons = 0x0F;
    dirButtons = 0x0F;
    
    // CPU defaults to no row selected
    joypadRegister = 0xFF; 
}

void Controller::pressButton(int button) {
    if (button < 4) {
        // Clear the specific bit (0 means pressed)
        dirButtons &= ~(1 << button);
    } else {
        actionButtons &= ~(1 << (button - 4));
    }
}

void Controller::releaseButton(int button) {
    if (button < 4) {
        // Set the specific bit back to 1 (Unpressed)
        dirButtons |= (1 << button);
    } else {
        actionButtons |= (1 << (button - 4));
    }
}

uint8_t Controller::read() const {
    // Bits 6 and 7 always return 1
    uint8_t output = joypadRegister | 0xCF; 
    
    // Bit 4 is 0: CPU wants Directional buttons
    if ((joypadRegister & 0x10) == 0) {
        output &= dirButtons;
    }
    
    // Bit 5 is 0: CPU wants Action buttons
    if ((joypadRegister & 0x20) == 0) {
        output &= actionButtons;
    }
    
    return output;
}

void Controller::write(uint8_t value) {
    // Only bits 4 and 5 are writable
    joypadRegister = (joypadRegister & 0xCF) | (value & 0x30);
}