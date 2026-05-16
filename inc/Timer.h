#pragma once
#include <cstdint>

// Forward declaration so the compiler knows the Memory class exists
class Memory; 

class Timer {
private:
    Memory& bus; // Reference to your memory bus so we can trigger interrupts

    // The Game Boy's actual hardware timer registers
    uint8_t div;  // 0xFF04 - Divider Register
    uint8_t tima; // 0xFF05 - Timer Counter
    uint8_t tma;  // 0xFF06 - Timer Modulo
    uint8_t tac;  // 0xFF07 - Timer Control

    // Internal hidden cycle trackers
    int dividerCounter;
    int timerCounter;

    // Helper to wake the CPU
    void requestInterrupt();

public:
    // Constructor (matches your Memory.cpp initialization!)
    Timer(Memory& memoryBus);
    
    // The three missing methods your architecture requires
    void step(int cycles);
    uint8_t read(uint16_t address) const;
    void write(uint16_t address, uint8_t value);
};