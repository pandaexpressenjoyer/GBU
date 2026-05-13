#include "CPU.h"
#include <cstdio>

CPU::CPU(Memory& mem) : memory(mem){
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

void CPU::setFlags(bool z, bool n, bool h, bool c) {
    f = 0;
    if (z) f |= 0x80; // Bit 7: Zero
    if (n) f |= 0x40; // Bit 6: Subtract
    if (h) f |= 0x20; // Bit 5: Half-carry
    if (c) f |= 0x10; // Bit 4: Carry
}

uint8_t CPU::fetchByte(){
    uint8_t value = memory.read(pc);
    pc++;
    return value;
}

uint16_t CPU::fetchWord(){
    // Game Boy is little-endian: lower byte is read first
    uint8_t lo = fetchByte();
    uint8_t hi = fetchByte();
    return (hi << 8) | lo;
}

int CPU::executeOpcode(uint8_t opcode){
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
            setFlags(a == 0, false, false, false);
            cycles = 4;
            break;

        case 0xCB: { // Prefix CB instruction catch
            uint8_t cbOpcode = fetchByte();
            cycles = executeCB(cbOpcode);
            break;
        }
            
        default:
            printf("Unimplemented opcode: 0x%02X at PC: 0x%04X\n", opcode, pc - 1);
            // exit(1); // Useful for stopping the emulator when hitting a missing instruction
            break;
    }

    return cycles;
}

void CPU::updateTimers(int cycles){
    // 1. Update DIV Register ($FF04)
    divCounter += cycles;
    if (divCounter >= 256){
        divCounter -= 256;
        uint8_t currentDiv = memory.read(0xFF04);
        memory.write(0xFF04, currentDiv + 1); 
    }

    // 2. Update TIMA Register ($FF05)
    uint8_t tac = memory.read(0xFF07);
    
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
            uint8_t tima = memory.read(0xFF05);
            
            if (tima == 0xFF) {
                // Overflow! Reset to TMA ($FF06) and request a Timer Interrupt
                memory.write(0xFF05, memory.read(0xFF06));
                requestInterrupt(2); // Bit 2 is the Timer Interrupt
            } else {
                memory.write(0xFF05, tima + 1);
            }
        }
    }
}

void CPU::requestInterrupt(int bit){
    // Read the Interrupt Flag register ($FF0F)
    uint8_t req = memory.read(0xFF0F);
    // Set the specific bit requested (0=VBlank, 1=LCD, 2=Timer, 3=Serial, 4=Joypad)
    req |= (1 << bit);
    memory.write(0xFF0F, req);
}

void CPU::handleInterrupts(){
    // If the Master Interrupt switch is on
    if (ime){
        uint8_t req = memory.read(0xFF0F);     // Pending interrupts
        uint8_t enable = memory.read(0xFFFF);  // Enabled interrupts (IE register)
        
        // If there are pending interrupts that are actually enabled
        if (req & enable){
            for (int i = 0; i < 5; i++) {
                if ((req & (1 << i)) && (enable & (1 << i))){
                    // 1. Disable the Master switch
                    ime = false; 
                    
                    // 2. Clear the specific interrupt flag that we are servicing
                    req &= ~(1 << i);
                    memory.write(0xFF0F, req);
                    
                    // 3. Push current Program Counter (PC) to the Stack
                    sp -= 2;
                    memory.write(sp, pc & 0xFF);         // Lower byte
                    memory.write(sp + 1, (pc >> 8) & 0xFF); // Upper byte
                    
                    // 4. Jump to the corresponding Interrupt Vector
                    pc = 0x0040 + (i * 0x08);
                    
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
    int cycles = executeOpcode(opcode); 

    // 3. Update hardware timers based on how long the instruction took
    updateTimers(cycles);

    return cycles;
}

int CPU::executeCB(uint8_t cbOpcode) {
    // 1. Extract target register (lowest 3 bits)
    // 0=B, 1=C, 2=D, 3=E, 4=H, 5=L, 6=(HL), 7=A
    uint8_t regIndex = cbOpcode & 0x07;
    
    uint8_t* targetReg = nullptr;
    uint8_t value = 0;
    
    switch (regIndex) {
        case 0: targetReg = &b; break;
        case 1: targetReg = &c; break;
        case 2: targetReg = &d; break;
        case 3: targetReg = &e; break;
        case 4: targetReg = &h; break;
        case 5: targetReg = &l; break;
        case 6: value = memory.read(hl); break; // Memory read for (HL)
        case 7: targetReg = &a; break;
    }

    if (targetReg != nullptr) {
        value = *targetReg;
    }

    // 2. Decode the operation
    uint8_t operation = (cbOpcode >> 6) & 0x03;   // Top 2 bits: 0=Rotates/Shifts, 1=BIT, 2=RES, 3=SET
    uint8_t bitToTarget = (cbOpcode >> 3) & 0x07; // Middle 3 bits
    
    // Cycle counting: Register ops = 8. (HL) ops = 16, except BIT (HL) which is 12.
    int cycles = 8;
    if (regIndex == 6) {
        cycles = (operation == 1) ? 12 : 16;
    }

    // Capture the current carry flag for instructions that need it
    bool currentCarry = (f & 0x10) != 0;
    bool carryOut = false;

    // 3. Execute the specific bitwise operation
    switch (operation) {
        case 0: // Shifts and Rotations
            switch (bitToTarget) {
                case 0: // RLC (Rotate Left Circular)
                    carryOut = (value & 0x80) != 0;
                    value = (value << 1) | (carryOut ? 1 : 0);
                    setFlags(value == 0, false, false, carryOut);
                    break;
                case 1: // RRC (Rotate Right Circular)
                    carryOut = (value & 0x01) != 0;
                    value = (value >> 1) | (carryOut ? 0x80 : 0);
                    setFlags(value == 0, false, false, carryOut);
                    break;
                case 2: // RL (Rotate Left through Carry)
                    carryOut = (value & 0x80) != 0;
                    value = (value << 1) | (currentCarry ? 1 : 0);
                    setFlags(value == 0, false, false, carryOut);
                    break;
                case 3: // RR (Rotate Right through Carry)
                    carryOut = (value & 0x01) != 0;
                    value = (value >> 1) | (currentCarry ? 0x80 : 0);
                    setFlags(value == 0, false, false, carryOut);
                    break;
                case 4: // SLA (Shift Left Arithmetic)
                    carryOut = (value & 0x80) != 0;
                    value = value << 1;
                    setFlags(value == 0, false, false, carryOut);
                    break;
                case 5: // SRA (Shift Right Arithmetic - Keep MSB)
                    carryOut = (value & 0x01) != 0;
                    value = (value >> 1) | (value & 0x80);
                    setFlags(value == 0, false, false, carryOut);
                    break;
                case 6: // SWAP (Swap upper and lower nibbles)
                    value = ((value & 0x0F) << 4) | ((value & 0xF0) >> 4);
                    setFlags(value == 0, false, false, false);
                    break;
                case 7: // SRL (Shift Right Logical - MSB becomes 0)
                    carryOut = (value & 0x01) != 0;
                    value = value >> 1;
                    setFlags(value == 0, false, false, carryOut);
                    break;
            }
            break;

        case 1: { // BIT (Test bit)
            bool isZero = (value & (1 << bitToTarget)) == 0;
            // BIT sets Zero, resets Subtract, sets Half-carry, leaves Carry alone
            setFlags(isZero, false, true, currentCarry);
            break;
        }

        case 2: // RES (Reset bit)
            value &= ~(1 << bitToTarget);
            break;

        case 3: // SET (Set bit)
            value |= (1 << bitToTarget);
            break;
    }

    // 4. Write the modified value back (BIT operations do not modify the value)
    if (operation != 1) {
        if (regIndex == 6) {
            memory.write(hl, value);
        } else {
            *targetReg = value;
        }
    }

    return cycles;
}