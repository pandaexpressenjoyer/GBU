#ifndef TIMER_H
#define TIMER_H

#include <cstdint>

// Forward declaration so the Timer knows Memory exists without looping
class Memory; 

class Timer {
public:
    Timer(Memory& mem);
    
    void step(int cycles);
    
    uint8_t read(uint16_t address) const;
    void write(uint16_t address, uint8_t value);

private:
    Memory& memory; 
    
    uint16_t divRegister; 
    int timerCounter;     
    
    uint8_t tima; 
    uint8_t tma;  
    uint8_t tac;  
    
    int getClockFreq();
};

#endif