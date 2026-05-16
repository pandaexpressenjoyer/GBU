#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include "Memory.h"

class CPU{
    public:
        CPU(Memory& mem);

        bool halted = false;
        bool stopped = false;

        int step();
        void reset();

        void requestInterrupt(int interruptBit);
        void handleInterrupts();
        
    private:
        Memory& memory;
        
        bool ime = false; // interrupt master enable flag
        
        union{
            struct{
                uint8_t f; // flags register
                uint8_t a; // accumulator
            };
            uint16_t af;
        };
        union{
            struct{
                uint8_t c; 
                uint8_t b; 
            };
            uint16_t bc;
        };
        union{
            struct{
                uint8_t e; 
                uint8_t d; 
            };
            uint16_t de;
        };
        union{
            struct{
                uint8_t l; 
                uint8_t h; 
            };
            uint16_t hl;
        };

        uint16_t pc; // program counter
        uint16_t sp; // stack pointer
        
        void setFlags(bool z, bool n, bool h, bool c);
        uint8_t fetchByte();
        uint16_t fetchWord();
        
        int executeOpcode(uint8_t opcode); 
        int executeCB(uint8_t cbOpcode); 
};

#endif