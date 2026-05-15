#include "Timer.h"
#include "Memory.h" 

Timer::Timer(Memory& mem) : memory(mem) {
    divRegister = 0;
    timerCounter = 1024;
    tima = 0;
    tma = 0;
    tac = 0;
}

void Timer::step(int cycles) {
    // 1. DIV Register constantly ticks
    divRegister += cycles;

    // 2. TIMA Register only ticks if Bit 2 of TAC is set to 1
    if (tac & 0x04) {
        timerCounter -= cycles;
        
        if (timerCounter <= 0) {
            timerCounter += getClockFreq(); 
            
            if (tima == 255) {
                // Overflow! Reset to TMA and request a Timer Interrupt
                tima = tma; 
                uint8_t interruptFlag = memory.read(0xFF0F);
                memory.write(0xFF0F, interruptFlag | 0x04);
            } else {
                tima++;
            }
        }
    }
}

uint8_t Timer::read(uint16_t address) const {
    switch (address) {
        case 0xFF04: return (divRegister >> 8); // Bitwise divider trick
        case 0xFF05: return tima;
        case 0xFF06: return tma;
        case 0xFF07: return tac;
    }
    return 0xFF;
}

void Timer::write(uint16_t address, uint8_t value) {
    switch (address) {
        case 0xFF04: 
            divRegister = 0; // Writing to DIV always resets it to 0
            break;
        case 0xFF05: 
            tima = value; 
            break;
        case 0xFF06: 
            tma = value; 
            break;
        case 0xFF07: {
            uint8_t currentFreq = tac & 0x03;
            tac = value; 
            if ((tac & 0x03) != currentFreq) {
                timerCounter = getClockFreq();
            }
            break;
        }
    }
}

int Timer::getClockFreq() {
    switch (tac & 0x03) {
        case 0x00: return 1024; 
        case 0x01: return 16;   
        case 0x02: return 64;   
        case 0x03: return 256;  
    }
    return 1024;
}