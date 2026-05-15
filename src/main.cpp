#include <iostream>
#include <SDL2/SDL.h>

#include "Controller.h"
#include "Memory.h"
#include "CPU.h"
#include "PPU.h"

// Game Boy Native Resolution
const int GB_WIDTH = 160;
const int GB_HEIGHT = 144;
const int SCALE = 4; // Scale the window 4x so it's easily visible

int main(int argc, char* argv[]) {
    // 1. Initialize Hardware Bus
    Controller joypad;
    Memory bus(joypad);
    CPU cpu(bus);
    PPU ppu(bus);

    // 2. Load the test ROM
    // Make sure 'cpu_instrs.gb' is in the same directory as your executable
    if (!bus.load("cpu_instrs.gb")) {
        std::cerr << "Failed to load ROM! Check the file path." << std::endl;
        return 1;
    }
    std::cout << "ROM loaded successfully. Booting Emulator..." << std::endl;

    // 3. Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Game Boy Emulator", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        GB_WIDTH * SCALE, GB_HEIGHT * SCALE, 
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    // The texture acts as our frame buffer. We write pixel data here.
    SDL_Texture* texture = SDL_CreateTexture(
        renderer, 
        SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING, 
        GB_WIDTH, GB_HEIGHT
    );

    // 4. The Main Game Loop
    bool isRunning = true;
    SDL_Event event;

    while (isRunning) {
        // --- Input Handling ---
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                isRunning = false;
            } 
            else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                bool isPressed = (event.type == SDL_KEYDOWN);
                
                // Map keyboard keys to Game Boy buttons
                // 0: Right, 1: Left, 2: Up, 3: Down | 4: A, 5: B, 6: Select, 7: Start
                int button = -1;
                switch (event.key.keysym.sym) {
                    case SDLK_RIGHT:  button = 0; break;
                    case SDLK_LEFT:   button = 1; break;
                    case SDLK_UP:     button = 2; break;
                    case SDLK_DOWN:   button = 3; break;
                    case SDLK_z:      button = 4; break; // Z = A
                    case SDLK_x:      button = 5; break; // X = B
                    case SDLK_LSHIFT: button = 6; break; // Shift = Select
                    case SDLK_RETURN: button = 7; break; // Enter = Start
                }

                if (button != -1) {
                    if (isPressed) joypad.pressButton(button);
                    else joypad.releaseButton(button);
                }
            }
        }

        // --- Hardware Stepping ---
        // Step the CPU and get how many clock cycles the instruction took
        int cycles = cpu.step();
        
        // Step the PPU and Timers to keep them in sync with the CPU
        ppu.step(cycles);
        bus.timer.step(cycles); // <-- ADDED: Step the hardware timers

        // --- Graphics Rendering ---
        // Only draw to the screen if the PPU has finished a full frame (VBlank)
        if (ppu.frameReady) {
            ppu.frameReady = false; // Reset the flag
            
            // 1. Copy the PPU's frameBuffer into the SDL Texture
            SDL_UpdateTexture(texture, nullptr, ppu.frameBuffer.data(), GB_WIDTH * sizeof(uint32_t));
            
            // 2. Clear the renderer, copy the texture over, and present it to the window
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
            
            // 3. Cap the frame rate roughly to 60fps (~16ms per frame)
            SDL_Delay(16); 
        }
    }

    // 5. Cleanup
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}