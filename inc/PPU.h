#ifndef PPU_H
#define PPU_H

#include <cstdint>
#include <array>
#include "Memory.h"

class PPU {
public:
    PPU(Memory& mem);
    
    void step(int cycles);

    std::array<uint32_t, 160 * 144> frameBuffer{};
    bool frameReady = false; 

    // The classic Game Boy color palette stays!
    const uint32_t colors[4] = {
        0xFFE0F8D0, // White
        0xFF88C070, // Light Gray
        0xFF346856, // Dark Gray
        0xFF081820  // Black
    };

    // Upgraded to accept a VRAM memory address
    void drawTile(uint16_t tileAddress, int startX, int startY);

private:
    Memory& memory;
    int scanlineCounter = 0;
    uint8_t currentScanline = 0;

    void setMode(uint8_t mode);
    void requestVBlankInterrupt();
};

#endif