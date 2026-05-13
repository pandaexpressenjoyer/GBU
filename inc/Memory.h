#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <string>
#include <array>

class Memory{
    public:
        Memory();
        uint8_t read(uint16_t address) const;
        void write(uint16_t address, uint8_t value);
        bool load(const std::string& path);
    private:
        std::array<uint8_t,0x10000> data{};
        std::array<uint8_t, 0x8000> rom{};
        uint8_t readIO(uint16_t address) const;
        void writeIO(uint16_t address, uint8_t value);
};

#endif