#include "Timer.h"
#include "Memory.h" // Full include here so we can call bus.read() / bus.write()

Timer::Timer(Memory& memoryBus) : bus(memoryBus) {
    div = 0;
    tima = 0;
    tma = 0;
    tac = 0;
    dividerCounter = 0;
    timerCounter = 0;
}

uint8_t Timer::read(uint16_t address) const {
    switch (address) {
        case 0xFF04: return div;
        case 0xFF05: return tima;
        case 0xFF06: return tma;
        case 0xFF07: return tac | 0xF8; // Unused upper bits of TAC always read as 1
        default: return 0xFF;
    }
}

void Timer::write(uint16_t address, uint8_t value) {
    switch (address) {
        case 0xFF04: 
            // HARDWARE RULE: Writing ANY value to DIV instantly resets it to 0
            div = 0; 
            dividerCounter = 0; // Also resets the internal clock ticks
            break;
        case 0xFF05: tima = value; break;
        case 0xFF06: tma = value; break;
        case 0xFF07: tac = value; break;
    }
}

void Timer::step(int cycles) {
    // --- 1. UPDATE THE DIVIDER REGISTER (DIV) ---
    dividerCounter += cycles;
    // The DIV register increments once every 256 CPU cycles
    if (dividerCounter >= 256) {
        dividerCounter -= 256;
        div++; 
    }

    // --- 2. UPDATE THE MAIN TIMER (TIMA) ---
    bool timerEnabled = (tac & 0x04) != 0; // Bit 2 checks if timer is ON
    
    if (timerEnabled) {
        timerCounter += cycles;

        // Find the frequency based on the bottom 2 bits of TAC
        int frequency = 0;
        switch (tac & 0x03) {
            case 0: frequency = 1024; break; // 4096 Hz
            case 1: frequency = 16;   break; // 262144 Hz
            case 2: frequency = 64;   break; // 65536 Hz
            case 3: frequency = 256;  break; // 16384 Hz
        }

        while (timerCounter >= frequency) {
            timerCounter -= frequency;
            
            if (tima == 0xFF) { 
                // TIMER OVERFLOW!
                tima = tma; // Reset to the value held in TMA
                requestInterrupt();
            } else {
                tima++; // Tick up normally
            }
        }
    }
}

void Timer::requestInterrupt() {
    // Read the current Interrupt Flag (IF) register from the Memory bus
    uint8_t currentIF = bus.read(0xFF0F);
    
    // Flip bit 2 to 1 (Bit 2 is the Timer Interrupt)
    bus.write(0xFF0F, currentIF | 0x04);
}