#include "PPU.h"

PPU::PPU(Memory& mem) : memory(mem){}

void PPU::step(int cycles){
    scanlineCounter += cycles;

    uint8_t stat = memory.read(0xFF41); // LCD Status Register
    uint8_t currentMode = stat & 0x03;

    // Scanline length is 456 CPU clock cycles
    if (scanlineCounter >= 456){
        scanlineCounter -= 456;

        currentScanline++;

        memory.write(0xFF44, currentScanline); // LY Register (Current Scanline)

        if (currentScanline == 144){
            // We entered VBlank! Request CPU Interrupt Bit 0.
            requestVBlankInterrupt();
        }
        else if (currentScanline > 153){
            // Back to the top of the screen
            currentScanline = 0;
            memory.write(0xFF44, 0);
        }
    }

    // --- State Machine ---
    if (currentScanline >= 144){
        setMode(1); // Mode 1: VBlank
    }
    else{
        if (scanlineCounter < 80){
            setMode(2); // Mode 2: OAM Search (Sprites)
        }
        else if (scanlineCounter < 252){
            setMode(3); // Mode 3: Pixel Transfer
        }
        else{
            setMode(0); // Mode 0: HBlank

            // (Optional: Draw the current scanline to your screen buffer here)
        }
    }
}

void PPU::setMode(uint8_t mode){
    uint8_t stat = memory.read(0xFF41);

    stat = (stat & ~0x03) | mode;

    memory.write(0xFF41, stat);
}

void PPU::requestVBlankInterrupt(){
    uint8_t flag = memory.read(0xFF0F);

    memory.write(0xFF0F, flag | 0x01); // Set bit 0
}