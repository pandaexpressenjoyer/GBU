#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <string>
#include <array>
#include "Controller.h"
#include "Cartridge.h" 
#include "Timer.h" // <-- Included the Timer

class Memory {
public:
    Memory(Controller& ctrl);

    bool load(const std::string& path);
    uint8_t read(uint16_t address) const;
    void write(uint16_t address, uint8_t value);

    Timer timer; // <-- Memory owns the timer

private:
    std::array<uint8_t, 0x10000> data; 
    Cartridge cart;                    
    Controller& controller; 

    uint8_t readIO(uint16_t address) const;
    void writeIO(uint16_t address, uint8_t value);
};

#endif