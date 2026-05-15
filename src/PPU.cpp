#include "PPU.h"
#include <cstdlib> // Added for rand()

PPU::PPU(Memory& mem) : memory(mem) {}

void PPU::step(int cycles) {
    scanlineCounter += cycles;
    uint8_t stat = memory.read(0xFF41); 
    uint8_t currentMode = stat & 0x03;
    
    if (scanlineCounter >= 456) {
        scanlineCounter -= 456;
        currentScanline++;
        memory.write(0xFF44, currentScanline); 

        if (currentScanline == 144) {
            // We entered VBlank! Request CPU Interrupt Bit 0.
            requestVBlankInterrupt(); 

            // --- TEMPORARY: GENERATE STATIC NOISE ---
            // Fill the frame buffer with random grayscale values
            for (int i = 0; i < 160 * 144; ++i) {
                uint8_t noise = rand() % 256;
                // Format is ARGB (0xAARRGGBB). Alpha must be 0xFF (fully opaque).
                frameBuffer[i] = 0xFF000000 | (noise << 16) | (noise << 8) | noise;
            }
            
            // Tell the main loop it is time to render!
            frameReady = true; 

        } else if (currentScanline > 153) {
            // Back to the top of the screen
            currentScanline = 0;
            memory.write(0xFF44, 0);
        }
    }

    // --- State Machine ---
    if (currentScanline >= 144) {
        setMode(1); // Mode 1: VBlank
    } else {
        if (scanlineCounter < 80) {
            setMode(2); // Mode 2: OAM Search (Sprites)
        } else if (scanlineCounter < 252) {
            setMode(3); // Mode 3: Pixel Transfer
        } else {
            setMode(0); // Mode 0: HBlank
        }
    }
}

void PPU::setMode(uint8_t mode) {
    uint8_t stat = memory.read(0xFF41);
    stat = (stat & ~0x03) | mode;
    memory.write(0xFF41, stat);
}

void PPU::requestVBlankInterrupt() {
    uint8_t flag = memory.read(0xFF0F);
    memory.write(0xFF0F, flag | 0x01); // Set bit 0
}