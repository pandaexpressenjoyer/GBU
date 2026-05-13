#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <string>
#include <array>
#include "Controller.h" // Required so Memory knows what a Controller is

class Memory{
    public:
        // Constructor now demands a Controller reference upon creation
        Memory(Controller& ctrl); 
        
        uint8_t read(uint16_t address) const;
        void write(uint16_t address, uint8_t value);
        bool load(const std::string& path);
        
    private:
        std::array<uint8_t,0x10000> data{};
        std::array<uint8_t, 0x8000> rom{};
        
        // The hardware reference we are routing to
        Controller& controller; 

        uint8_t readIO(uint16_t address) const;
        void writeIO(uint16_t address, uint8_t value);
};

#endif