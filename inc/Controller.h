#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <cstdint>

class Controller{
public:
    Controller();

    // Called by your emulator's event loop when a keyboard key is pressed/released
    void pressButton(int button);
    void releaseButton(int button);

    // Called by Memory::write(0xFF00, value)
    void write(uint8_t value);

    // Called by Memory::read(0xFF00)
    uint8_t read() const;

    bool checkInterrupt();

private:
    // Bits 0-3: Right, Left, Up, Down | Bits 4-7: A, B, Select, Start
    uint8_t state = 0xFF;

    // Bits 4 and 5
    uint8_t selectBits = 0x30;

    bool requestInterrupt = false;
};

#endif