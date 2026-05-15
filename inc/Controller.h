#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <cstdint>

class Controller {
public:
    Controller();

    // Called by main.cpp when an SDL key event occurs
    void pressButton(int button);
    void releaseButton(int button);

    // Called by the CPU via the Memory bus at 0xFF00
    uint8_t read() const;
    void write(uint8_t value);

private:
    // 1 = unpressed, 0 = pressed
    uint8_t actionButtons; // A, B, Select, Start
    uint8_t dirButtons;    // Right, Left, Up, Down
    
    // Stores the row selection (Bit 4 = D-Pad, Bit 5 = Action)
    uint8_t joypadRegister; 
};

#endif