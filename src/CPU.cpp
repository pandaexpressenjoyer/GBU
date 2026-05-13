#include "CPU.h"

CPU::CPU(Memory& memory) : mem(memory){
    reset();
}

void CPU::reset(){
    // Initial state of the CPU after the bootrom finishes
    af = 0x01B0;
    bc = 0x0013;
    de = 0x00D8;
    hl = 0x014D;
    sp = 0xFFFE;
    pc = 0x0100; // 0x0100 is the entry point of the game cartridge
}

uint8_t CPU::fetchByte(){
    uint8_t value = mem.read(pc);
    pc++;
    return value;
}

uint16_t CPU::fetchWord(){
    // Game Boy is little-endian: lower byte is read first
    uint8_t lo = fetchByte();
    uint8_t hi = fetchByte();
    return (hi << 8) | lo;
}

int CPU::step(){
    uint8_t opcode = fetchByte();
    int cycles = 0;

    switch (opcode){
        case 0x00: // NOP
            cycles = 4;
            break;

        case 0xC3:{ // JP a16 (Jump to 16-bit address)
            uint16_t address = fetchWord();
            pc = address;
            cycles = 16;
            break;
        }

        case 0x31:{ // LD SP, d16 (Load 16-bit immediate into SP)
            sp = fetchWord();
            cycles = 12;
            break;
        }

        case 0xAF: // XOR A
            a ^= a;
            // Set flags: Zero flag on (if result is 0), others off
            f = (a == 0) ? 0x80 : 0x00;
            cycles = 4;
            break;
        default:
            printf("Unimplemented opcode: 0x%02X at PC: 0x%04X\n", opcode, pc - 1);
            // exit(1); // Useful for stopping the emulator when hitting a missing instruction
            break;
    }

    return cycles;
}

void CPU::updateTimers(int cycles){
    // 1. Update DIV Register ($FF04)
    // Pan Docs specifies DIV increments at 16384 Hz. 
    // On a 4.194304 MHz CPU, this happens every 256 clock cycles.
    divCounter += cycles;
    if (divCounter >= 256){
        divCounter -= 256;
        uint8_t currentDiv = mem.read(0xFF04);
        // Note: The Memory class should reset DIV to 0 if a game tries to WRITE to it,
        // but the CPU internally increments it normally.
        mem.write(0xFF04, currentDiv + 1); 
    }

    // 2. Update TIMA Register ($FF05)
    uint8_t tac = mem.read(0xFF07);
    
    // Check if the timer is enabled (Bit 2 of TAC register)
    if (tac & 0x04){
        timerCounter += cycles;
        
        // Determine the frequency based on TAC bits 1-0
        int frequency = 0;
        switch (tac & 0x03){
            case 0x00: frequency = 1024; break; // 4096 Hz (4.19MHz / 4096 = 1024)
            case 0x01: frequency = 16;   break; // 262144 Hz
            case 0x02: frequency = 64;   break; // 65536 Hz
            case 0x03: frequency = 256;  break; // 16384 Hz
        }

        // If enough clock cycles have passed, increment TIMA
        if (timerCounter >= frequency){
            timerCounter -= frequency;
            uint8_t tima = mem.read(0xFF05);
            
            if (tima == 0xFF) {
                // Overflow! Reset to TMA ($FF06) and request a Timer Interrupt
                mem.write(0xFF05, mem.read(0xFF06));
                requestInterrupt(2); // Bit 2 is the Timer Interrupt
            } else {
                mem.write(0xFF05, tima + 1);
            }
        }
    }
}

void CPU::requestInterrupt(int bit){
    // Read the Interrupt Flag register ($FF0F)
    uint8_t req = mem.read(0xFF0F);
    // Set the specific bit requested (0=VBlank, 1=LCD, 2=Timer, 3=Serial, 4=Joypad)
    req |= (1 << bit);
    mem.write(0xFF0F, req);
}

void CPU::handleInterrupts(){
    // If the Master Interrupt switch is on
    if (ime){
        uint8_t req = mem.read(0xFF0F);     // Pending interrupts
        uint8_t enable = mem.read(0xFFFF);  // Enabled interrupts (IE register)
        
        // If there are pending interrupts that are actually enabled
        if (req & enable){
            for (int i = 0; i < 5; i++) {
                if ((req & (1 << i)) && (enable & (1 << i))){
                    // 1. Disable the Master switch
                    ime = false; 
                    
                    // 2. Clear the specific interrupt flag that we are servicing
                    req &= ~(1 << i);
                    mem.write(0xFF0F, req);
                    
                    // 3. Push current Program Counter (PC) to the Stack
                    sp -= 2;
                    mem.write(sp, pc & 0xFF);         // Lower byte
                    mem.write(sp + 1, (pc >> 8) & 0xFF); // Upper byte
                    
                    // 4. Jump to the corresponding Interrupt Vector
                    // Pan Docs specifies: VBlank=0x40, LCD=0x48, Timer=0x50, Serial=0x58, Joypad=0x60
                    pc = 0x0040 + (i * 0x08);
                    
                    // Servicing an interrupt takes 5 machine cycles (20 clock cycles)
                    // You might want to account for this in your central loop
                    break; 
                }
            }
        }
    }
}

int CPU::step(){
    // 1. Check for hardware interrupts before fetching the next instruction
    handleInterrupts();

    // 2. Fetch, decode, and execute the opcode
    uint8_t opcode = fetchByte();
    int cycles = executeOpcode(opcode); // Your giant switch statement goes here

    // 3. Update hardware timers based on how long the instruction took
    updateTimers(cycles);

    return cycles;
}