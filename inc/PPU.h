#ifndef PPU_H
#define PPU_H

#include <cstdint>
#include <array>
#include "Memory.h"

class PPU {
public:
    PPU(Memory& mem);
    
    void step(int cycles);

    // The screen buffer (160 width * 144 height)
    std::array<uint32_t, 160 * 144> frameBuffer{};
    
    // Flag to tell main.cpp to draw the screen
    bool frameReady = false; 

private:
    Memory& memory;
    int scanlineCounter = 0;
    uint8_t currentScanline = 0;

    void setMode(uint8_t mode);
    void requestVBlankInterrupt();
};

#endif