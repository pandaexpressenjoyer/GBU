#include "PPU.h"

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
            requestVBlankInterrupt(); 

            // In the final emulator, the screen is already fully drawn by the 
            // time we hit VBlank. We just tell main.cpp to push it to SDL!
            frameReady = true; 

        } else if (currentScanline > 153) {
            currentScanline = 0;
            memory.write(0xFF44, 0);
        }
    }

    // --- State Machine ---
    if (currentScanline >= 144) {
        setMode(1); 
    } else {
        if (scanlineCounter < 80) setMode(2); 
        // Later, we will put the actual scanline drawing logic inside Mode 3 here!
        else if (scanlineCounter < 252) setMode(3); 
        else setMode(0); 
    }
}

void PPU::setMode(uint8_t mode) {
    uint8_t stat = memory.read(0xFF41);
    stat = (stat & ~0x03) | mode;
    memory.write(0xFF41, stat);
}

void PPU::requestVBlankInterrupt() {
    uint8_t flag = memory.read(0xFF0F);
    memory.write(0xFF0F, flag | 0x01);
}

// --- THE UPGRADED TILE DECODER ---
void PPU::drawTile(uint16_t tileAddress, int startX, int startY) {
    for (int row = 0; row < 8; row++) {
        
        // Calculate the memory address for this specific row (2 bytes per row)
        uint16_t rowAddress = tileAddress + (row * 2);
        
        // Read the actual Game Boy VRAM memory instead of the hardcoded array!
        uint8_t byte1 = memory.read(rowAddress);     
        uint8_t byte2 = memory.read(rowAddress + 1); 
        
        for (int pixel = 0; pixel < 8; pixel++) {
            int bitIndex = 7 - pixel;
            
            uint8_t lowerBit = (byte1 >> bitIndex) & 0x01;
            uint8_t upperBit = (byte2 >> bitIndex) & 0x01;
            
            uint8_t colorId = (upperBit << 1) | lowerBit;
            
            int bufferX = startX + pixel;
            int bufferY = startY + row;
            
            if (bufferX >= 0 && bufferX < 160 && bufferY >= 0 && bufferY < 144) {
                frameBuffer[(bufferY * 160) + bufferX] = colors[colorId];
            }
        }
    }
}