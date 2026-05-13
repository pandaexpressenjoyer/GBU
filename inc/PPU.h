#ifndef PPU_H
#define PPU_H

#include <cstdint>
#include "Memory.h"

class PPU{
public:
    PPU(Memory& mem);

    void step(int cycles);

private:
    Memory& memory;

    int scanlineCounter = 0;
    uint8_t currentScanline = 0;

    void setMode(uint8_t mode);
    void requestVBlankInterrupt();
};

#endif