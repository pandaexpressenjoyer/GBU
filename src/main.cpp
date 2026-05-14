#include <iostream>
#include <SDL2/SDL.h> 

#include "Controller.h"
#include "Memory.h"
#include "CPU.h"
#include "PPU.h"

// Game Boy Native Resolution
const int GB_WIDTH = 160;
const int GB_HEIGHT = 144;
const int SCALE = 4; // Scale the window 4x so it's visible on modern screens

int main(int argc, char* argv[]) {
    // 1. Initialize Hardware Bus
    Controller joypad;
    Memory bus(joypad);
    CPU cpu(bus);
    PPU ppu(bus);

    // Load ROM (replace with your path)
    if (!bus.load("cpu_instrs.gb")) {
        std::cerr << "Failed to load ROM!" << std::endl;
        return 1;
    }

    // 2. Initialize SDL2
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
    
    // The texture acts as our frame buffer. We will eventually write pixel data to this.
    SDL_Texture* texture = SDL_CreateTexture(
        renderer, 
        SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING, 
        GB_WIDTH, GB_HEIGHT
    );

    // A temporary array to hold our pixel colors (0xAARRGGBB)
    uint32_t screenPixels[GB_WIDTH * GB_HEIGHT] = {0};

    // 3. The Main Game Loop
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
                // Bit mapping based on your Controller class:
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
        // Normally, you would run a loop here to execute enough CPU cycles 
        // to equal one frame (~70,224 clock cycles per frame).
        // For now, we will just step it once per loop so it doesn't hang.
        int cycles = cpu.step();
        ppu.step(cycles);

        // --- Graphics Rendering ---
        // (Eventually, your PPU will fill screenPixels with actual game data)
        
        // Update the texture with our pixel array
        SDL_UpdateTexture(texture, nullptr, screenPixels, GB_WIDTH * sizeof(uint32_t));
        
        // Clear screen, copy texture, and present it
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
        
        // Cap the frame rate roughly to 60fps (~16ms per frame)
        SDL_Delay(16); 
    }

    // 4. Cleanup
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}