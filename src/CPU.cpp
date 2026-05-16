#include "CPU.h"
#include <cstdio>
#include <cstdlib>

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
    ime = false; 
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

        case 0x31:{ // LD SP, d16 (Load 16-bit immediate into SP)
            sp = fetchWord();
            cycles = 12;
            break;
        }

        case 0xAF: // XOR A
            a ^= a;
            setFlags(a == 0, false, false, false);
            cycles = 4;
            break;

        // --- STACK OPERATIONS ---
        case 0xC1: { // POP BC
            c = memory.read(sp);
            b = memory.read(sp + 1);
            sp += 2;
            cycles = 12;
            break;
        }

        case 0xC5: { // PUSH BC
            sp -= 2;
            memory.write(sp, c);      // Lower byte
            memory.write(sp + 1, b);  // Upper byte
            cycles = 16;
            break;
        }

        // --- JUMPS, CALLS, AND RETURNS ---
        case 0xC3:{ // JP a16 (Jump to 16-bit address)
            uint16_t address = fetchWord();
            pc = address;
            cycles = 16;
            break;
        }

        case 0xC9: { // RET
            uint8_t lo = memory.read(sp);
            uint8_t hi = memory.read(sp + 1);
            pc = (hi << 8) | lo;
            sp += 2;
            cycles = 16;
            break;
        }

        case 0xCD: { // CALL a16
            uint16_t targetAddress = fetchWord();
            sp -= 2;
            memory.write(sp, pc & 0xFF);            // Lower byte of PC
            memory.write(sp + 1, (pc >> 8) & 0xFF); // Upper byte of PC
            pc = targetAddress;
            cycles = 24;
            break;
        }

        // --- INTERRUPT CONTROL ---
        case 0xD9: { // RETI (Return from Interrupt)
            uint8_t lo = memory.read(sp);
            uint8_t hi = memory.read(sp + 1);
            pc = (hi << 8) | lo;
            sp += 2;
            ime = true; // Re-enable Master Interrupt flag
            cycles = 16;
            break;
        }

        case 0xF3: // DI (Disable Interrupts)
            ime = false;
            cycles = 4;
            break;

        case 0xFB: // EI (Enable Interrupts)
            ime = true; 
            cycles = 4;
            break;

        // --- MEMORY LOADS ---
        case 0xEA: { // LD (a16), A
            uint16_t address = fetchWord();
            memory.write(address, a);
            cycles = 16; 
            break;
        }
        case 0x3E: { // LD A, d8
            // Fetch the next byte from memory and put it in register A
            a = fetchByte();
            
            cycles = 8; // This takes 8 clock cycles
            break;
        }
        case 0xE0: { // LDH (a8), A
            // Fetch the 8-bit offset
            uint8_t offset = fetchByte();
            
            // Write register A to the hardware I/O address (0xFF00 + offset)
            memory.write(0xFF00 + offset, a);
            
            cycles = 12; // This takes 12 clock cycles
            break;
        }
        case 0x21: { // LD HL, d16
            // Fetch the 16-bit value and store it in the HL register pair
            hl = fetchWord();
            
            cycles = 12; // Takes 12 clock cycles
            break;
        }
        case 0x7D: { // LD A, L
            // Copy the value from register L into register A
            a = l;
            
            cycles = 4; // This is a very fast instruction, taking only 4 clock cycles
            break;
        }
        case 0x7C: { // LD A, H
            // Copy the value from register H into register A
            a = h;
            
            cycles = 4; // Takes 4 clock cycles
            break;
        }
        // --- 8-BIT LOADS INTO REGISTER A ---
        case 0x78: { // LD A, B
            a = b; 
            cycles = 4; 
            break; 
        }
        case 0x79: { // LD A, C
            a = c; 
            cycles = 4; 
            break; 
        }
        case 0x7A: { // LD A, D
            a = d; 
            cycles = 4; 
            break; 
        }
        case 0x7B: { // LD A, E
            a = e; 
            cycles = 4; 
            break; 
        }
        
        case 0x7E: { // LD A, (HL)
            a = memory.read(hl); 
            cycles = 8; // Takes longer because it has to read memory
            break; 
        }
        case 0x7F: { // LD A, A
            a = a; // Yes, games actually use this sometimes as a tiny delay!
            cycles = 4; 
            break; 
        }
        case 0x18: { // JR r8 (Jump Relative)
            // Fetch the 8-bit offset and cast it to a signed integer
            int8_t offset = (int8_t)fetchByte();
            
            // Add the offset to the Program Counter
            pc += offset;
            
            cycles = 12; // This takes 12 clock cycles
            break;
        }
        case 0xE5: { // PUSH HL
            // Decrement the Stack Pointer by 2
            sp -= 2;
            
            // Push the HL register pair onto the stack (Little-Endian)
            memory.write(sp, l);      // Lower byte
            memory.write(sp + 1, h);  // Upper byte
            
            cycles = 16; // Takes 16 clock cycles
            break;
        }
        case 0xE1: { // POP HL
            // Pop the 16-bit value off the stack into the HL register pair
            l = memory.read(sp);
            h = memory.read(sp + 1);
            
            // Increment the Stack Pointer by 2
            sp += 2;
            
            cycles = 12; // Takes 12 clock cycles
            break;
        }
        case 0xF5: { // PUSH AF
            // Decrement the Stack Pointer by 2
            sp -= 2;
            
            // Push the AF register pair onto the stack (Little-Endian)
            memory.write(sp, f);      // Lower byte (Flags)
            memory.write(sp + 1, a);  // Upper byte (Accumulator)
            
            cycles = 16; // Takes 16 clock cycles
            break;
        }
        // --- 16-BIT INCREMENTS ---
        case 0x03: { // INC BC
            bc++;
            cycles = 8;
            break;
        }
        case 0x13: { // INC DE
            de++;
            cycles = 8;
            break;
        }
        case 0x23: { // INC HL
            hl++;
            cycles = 8;
            break;
        }
        case 0x33: { // INC SP
            sp++;
            cycles = 8;
            break;
        }
        // --- AUTO INCREMENT/DECREMENT MEMORY LOADS ---
        case 0x2A: { // LDI A, (HL)  [or LD A, (HL+)]
            // Read value at memory address HL into A
            a = memory.read(hl);
            // Increment HL
            hl++;
            cycles = 8;
            break;
        }
        // --- 16-BIT DECREMENTS ---
        // Note: 16-bit decrements DO NOT modify ANY flags!
        case 0x0B: { // DEC BC
            bc--;
            cycles = 8;
            break;
        }
        case 0x1B: { // DEC DE
            de--;
            cycles = 8;
            break;
        }
        case 0x2B: { // DEC HL
            hl--;
            cycles = 8;
            break;
        }
        case 0x3B: { // DEC SP
            sp--;
            cycles = 8;
            break;
        }
        case 0x22: { // LDI (HL), A  [or LD (HL+), A]
            // Write value of A into memory address HL
            memory.write(hl, a);
            // Increment HL
            hl++;
            cycles = 8;
            break;
        }
        // --- 8-BIT ALU: SUBTRACT WITH CARRY (SBC) ---
        case 0x98: { uint8_t carry = (f & 0x10) ? 1 : 0; int res = a - b - carry; int hRes = (a & 0x0F) - (b & 0x0F) - carry; a = (uint8_t)res; setFlags(a == 0, true, hRes < 0, res < 0); cycles = 4; break; }
        case 0x99: { uint8_t carry = (f & 0x10) ? 1 : 0; int res = a - c - carry; int hRes = (a & 0x0F) - (c & 0x0F) - carry; a = (uint8_t)res; setFlags(a == 0, true, hRes < 0, res < 0); cycles = 4; break; }
        case 0x9A: { uint8_t carry = (f & 0x10) ? 1 : 0; int res = a - d - carry; int hRes = (a & 0x0F) - (d & 0x0F) - carry; a = (uint8_t)res; setFlags(a == 0, true, hRes < 0, res < 0); cycles = 4; break; }
        case 0x9B: { uint8_t carry = (f & 0x10) ? 1 : 0; int res = a - e - carry; int hRes = (a & 0x0F) - (e & 0x0F) - carry; a = (uint8_t)res; setFlags(a == 0, true, hRes < 0, res < 0); cycles = 4; break; }
        case 0x9C: { uint8_t carry = (f & 0x10) ? 1 : 0; int res = a - h - carry; int hRes = (a & 0x0F) - (h & 0x0F) - carry; a = (uint8_t)res; setFlags(a == 0, true, hRes < 0, res < 0); cycles = 4; break; }
        case 0x9D: { uint8_t carry = (f & 0x10) ? 1 : 0; int res = a - l - carry; int hRes = (a & 0x0F) - (l & 0x0F) - carry; a = (uint8_t)res; setFlags(a == 0, true, hRes < 0, res < 0); cycles = 4; break; }
        case 0x9E: { uint8_t val = memory.read(hl); uint8_t carry = (f & 0x10) ? 1 : 0; int res = a - val - carry; int hRes = (a & 0x0F) - (val & 0x0F) - carry; a = (uint8_t)res; setFlags(a == 0, true, hRes < 0, res < 0); cycles = 8; break; }
        case 0x9F: { uint8_t carry = (f & 0x10) ? 1 : 0; int res = a - a - carry; int hRes = (a & 0x0F) - (a & 0x0F) - carry; a = (uint8_t)res; setFlags(a == 0, true, hRes < 0, res < 0); cycles = 4; break; }
        case 0xDE: { // SBC A, d8
            uint8_t val = fetchByte();
            uint8_t carry = (f & 0x10) ? 1 : 0;
            int res = a - val - carry;
            int hRes = (a & 0x0F) - (val & 0x0F) - carry;
            a = (uint8_t)res;
            setFlags(a == 0, true, hRes < 0, res < 0);
            cycles = 8;
            break;
        }
        case 0x3A: { // LDD A, (HL)  [or LD A, (HL-)]
            // Read value at memory address HL into A
            a = memory.read(hl);
            // Decrement HL
            hl--;
            cycles = 8;
            break;
        }
        // --- RESTART (RST) INSTRUCTIONS ---
        // These are fast, 1-byte CALL instructions to fixed memory addresses.
        case 0xC7: { // RST 00H
            memory.write(--sp, pc >> 8);   // Push high byte of PC
            memory.write(--sp, pc & 0xFF); // Push low byte of PC
            pc = 0x0000;
            cycles = 16;
            break;
        }
        case 0xCF: { // RST 08H
            memory.write(--sp, pc >> 8);
            memory.write(--sp, pc & 0xFF);
            pc = 0x0008;
            cycles = 16;
            break;
        }
        case 0xD7: { // RST 10H
            memory.write(--sp, pc >> 8);
            memory.write(--sp, pc & 0xFF);
            pc = 0x0010;
            cycles = 16;
            break;
        }
        case 0xDF: { // RST 18H
            memory.write(--sp, pc >> 8);
            memory.write(--sp, pc & 0xFF);
            pc = 0x0018;
            cycles = 16;
            break;
        }
        case 0xE7: { // RST 20H
            memory.write(--sp, pc >> 8);
            memory.write(--sp, pc & 0xFF);
            pc = 0x0020;
            cycles = 16;
            break;
        }
        case 0xEF: { // RST 28H
            memory.write(--sp, pc >> 8);
            memory.write(--sp, pc & 0xFF);
            pc = 0x0028;
            cycles = 16;
            break;
        }
        case 0xF7: { // RST 30H
            memory.write(--sp, pc >> 8);
            memory.write(--sp, pc & 0xFF);
            pc = 0x0030;
            cycles = 16;
            break;
        }
        case 0xFF: { // RST 38H
            memory.write(--sp, pc >> 8);
            memory.write(--sp, pc & 0xFF);
            pc = 0x0038;
            cycles = 16;
            break;
        }
        case 0x32: { // LDD (HL), A  [or LD (HL-), A]
            // Write value of A into memory address HL
            memory.write(hl, a);
            // Decrement HL
            hl--;
            cycles = 8;
            break;
        }
        case 0xE2: { // LD (C), A 
            // Write the value in A to the hardware address (0xFF00 + C)
            memory.write(0xFF00 + c, a);
            cycles = 8;
            break;
        }
        case 0xF2: { // LD A, (C)
            // Read the value from the hardware address (0xFF00 + C) into A
            a = memory.read(0xFF00 + c);
            cycles = 8;
            break;
        }
        // --- ACCUMULATOR LOGIC ---
        case 0x2F: { // CPL (Complement Accumulator)
            // Flip all bits in A
            a = ~a;
            
            // Flags: Z is preserved, N is set, H is set, C is preserved
            bool currentZ = (f & 0x80) != 0;
            bool currentC = (f & 0x10) != 0;
            setFlags(currentZ, true, true, currentC);
            
            cycles = 4;
            break;
        }
        
        // --- THE FINAL STACK OPERATIONS ---
        case 0xF1: { // POP AF
            // Read lower byte into Flags (enforcing the bottom 4 bits to be 0!)
            f = memory.read(sp) & 0xF0; 
            // Read upper byte into Accumulator
            a = memory.read(sp + 1);
            
            // Increment the Stack Pointer by 2
            sp += 2;
            
            cycles = 12; // Takes 12 clock cycles
            break;
        }
        // --- CARRY FLAG OPERATIONS ---
        case 0x37: { // SCF (Set Carry Flag)
            // Preserve Z, clear N and H, set C to 1
            bool currentZ = (f & 0x80) != 0;
            setFlags(currentZ, false, false, true);
            
            cycles = 4;
            break;
        }
        
        case 0x3F: { // CCF (Complement Carry Flag)
            // Preserve Z, clear N and H, flip current C
            bool currentZ = (f & 0x80) != 0;
            bool currentC = (f & 0x10) != 0;
            setFlags(currentZ, false, false, !currentC);
            
            cycles = 4;
            break;
        }
        // --- 8-BIT ALU: LOGICAL XOR ---
        case 0xA8: { // XOR B
            a ^= b; 
            setFlags(a == 0, false, false, false); 
            cycles = 4; 
            break; 
        }
        case 0xA9: { // XOR C
            a ^= c; 
            setFlags(a == 0, false, false, false); 
            cycles = 4; 
            break; 
        }
        case 0xAA: { // XOR D
            a ^= d; 
            setFlags(a == 0, false, false, false); 
            cycles = 4; 
            break; 
        }
        case 0xAB: { // XOR E
            a ^= e; 
            setFlags(a == 0, false, false, false); 
            cycles = 4; 
            break; 
        }
        case 0xAC: { // XOR H
            a ^= h; 
            setFlags(a == 0, false, false, false); 
            cycles = 4; 
            break; 
        }
        case 0xAD: { // XOR L
            a ^= l; 
            setFlags(a == 0, false, false, false); 
            cycles = 4; 
            break; 
        }
        case 0xAE: { // XOR (HL)
            a ^= memory.read(hl); 
            setFlags(a == 0, false, false, false); 
            cycles = 8; 
            break; 
        }
        
        case 0xD5: { // PUSH DE
            sp -= 2;
            memory.write(sp, e);      // Lower byte
            memory.write(sp + 1, d);  // Upper byte
            cycles = 16;
            break;
        }

        case 0xD1: { // POP DE
            e = memory.read(sp);
            d = memory.read(sp + 1);
            sp += 2;
            cycles = 12;
            break;
        }
        // --- 16-BIT IMMEDIATE LOADS ---
        case 0x01: { // LD BC, d16
            bc = fetchWord();
            cycles = 12;
            break;
        }

        case 0x11: { // LD DE, d16
            de = fetchWord();
            cycles = 12;
            break;
        }
        // --- 8-BIT ALU: BITWISE OR ---
        case 0xB0: { // OR B
            a |= b;
            setFlags(a == 0, false, false, false);
            cycles = 4;
            break;
        }
        case 0xB1: { // OR C
            a |= c;
            setFlags(a == 0, false, false, false);
            cycles = 4;
            break;
        }
        case 0xB2: { // OR D
            a |= d;
            setFlags(a == 0, false, false, false);
            cycles = 4;
            break;
        }
        case 0xB3: { // OR E
            a |= e;
            setFlags(a == 0, false, false, false);
            cycles = 4;
            break;
        }
        case 0xB4: { // OR H
            a |= h;
            setFlags(a == 0, false, false, false);
            cycles = 4;
            break;
        }
        case 0xB5: { // OR L
            a |= l;
            setFlags(a == 0, false, false, false);
            cycles = 4;
            break;
        }
        case 0xB6: { // OR (HL)
            a |= memory.read(hl);
            setFlags(a == 0, false, false, false);
            cycles = 8; // Slower because it reads from memory
            break;
        }
        case 0xB7: { // OR A
            a |= a; // Functionally just checks if A is zero and clears the other flags!
            setFlags(a == 0, false, false, false);
            cycles = 4;
            break;
        }
        // --- CONDITIONAL RELATIVE JUMPS ---
        case 0x20: { // JR NZ, r8 (Jump if Not Zero)
            int8_t offset = (int8_t)fetchByte();
            if ((f & 0x80) == 0) { // If Zero flag (bit 7) is OFF
                pc += offset;
                cycles = 12; // Takes longer if the jump happens
            } else {
                cycles = 8;  // Faster if it ignores the jump
            }
            break;
        }
        
        case 0x28: { // JR Z, r8 (Jump if Zero)
            int8_t offset = (int8_t)fetchByte();
            if ((f & 0x80) != 0) { // If Zero flag (bit 7) is ON
                pc += offset;
                cycles = 12;
            } else {
                cycles = 8;
            }
            break;
        }

        case 0x30: { // JR NC, r8 (Jump if No Carry)
            int8_t offset = (int8_t)fetchByte();
            if ((f & 0x10) == 0) { // If Carry flag (bit 4) is OFF
                pc += offset;
                cycles = 12;
            } else {
                cycles = 8;
            }
            break;
        }

        case 0x38: { // JR C, r8 (Jump if Carry)
            int8_t offset = (int8_t)fetchByte();
            if ((f & 0x10) != 0) { // If Carry flag (bit 4) is ON
                pc += offset;
                cycles = 12;
            } else {
                cycles = 8;
            }
            break;
        }
        case 0xF0: { // LDH A, (a8)
            // Fetch the 8-bit offset
            uint8_t offset = fetchByte();
            
            // Read the hardware I/O address (0xFF00 + offset) into register A
            a = memory.read(0xFF00 + offset);
            
            cycles = 12; // This takes 12 clock cycles
            break;
        }
        // --- 8-BIT ALU: COMPARE (CP) ---
        // CP is a subtraction that updates flags but DOES NOT modify register A.
        case 0xB8: { // CP B
            setFlags(a == b, true, (a & 0x0F) < (b & 0x0F), a < b);
            cycles = 4;
            break;
        }
        case 0xB9: { // CP C
            setFlags(a == c, true, (a & 0x0F) < (c & 0x0F), a < c);
            cycles = 4;
            break;
        }
        case 0xBA: { // CP D
            setFlags(a == d, true, (a & 0x0F) < (d & 0x0F), a < d);
            cycles = 4;
            break;
        }
        case 0xBB: { // CP E
            setFlags(a == e, true, (a & 0x0F) < (e & 0x0F), a < e);
            cycles = 4;
            break;
        }
        case 0xBC: { // CP H
            setFlags(a == h, true, (a & 0x0F) < (h & 0x0F), a < h);
            cycles = 4;
            break;
        }
        case 0xBD: { // CP L
            setFlags(a == l, true, (a & 0x0F) < (l & 0x0F), a < l);
            cycles = 4;
            break;
        }
        case 0xBE: { // CP (HL)
            uint8_t value = memory.read(hl);
            setFlags(a == value, true, (a & 0x0F) < (value & 0x0F), a < value);
            cycles = 8;
            break;
        }
        case 0xBF: { // CP A
            // A compared to A will always be equal, and will never borrow!
            setFlags(true, true, false, false); 
            cycles = 4;
            break;
        }
        case 0xFE: { // CP d8
            uint8_t value = fetchByte();
            setFlags(a == value, true, (a & 0x0F) < (value & 0x0F), a < value);
            cycles = 8;
            break;
        }
        case 0xFA: { // LD A, (a16)
            // Fetch the 16-bit target memory address
            uint16_t address = fetchWord();
            
            // Read the value from that memory address into register A
            a = memory.read(address);
            
            cycles = 16; // Takes 16 clock cycles (Fetch opcode, fetch 2 address bytes, read memory)
            break;
        }
        // --- 8-BIT ALU: BITWISE AND ---
        // AND operations always set the Half-Carry (H) flag to true!
        case 0xA0: { // AND B
            a &= b;
            setFlags(a == 0, false, true, false);
            cycles = 4;
            break;
        }
        case 0xA1: { // AND C
            a &= c;
            setFlags(a == 0, false, true, false);
            cycles = 4;
            break;
        }
        case 0xA2: { // AND D
            a &= d;
            setFlags(a == 0, false, true, false);
            cycles = 4;
            break;
        }
        case 0xA3: { // AND E
            a &= e;
            setFlags(a == 0, false, true, false);
            cycles = 4;
            break;
        }
        case 0xA4: { // AND H
            a &= h;
            setFlags(a == 0, false, true, false);
            cycles = 4;
            break;
        }
        case 0xA5: { // AND L
            a &= l;
            setFlags(a == 0, false, true, false);
            cycles = 4;
            break;
        }
        case 0xA6: { // AND (HL)
            a &= memory.read(hl);
            setFlags(a == 0, false, true, false);
            cycles = 8;
            break;
        }
        case 0xA7: { // AND A
            a &= a; 
            setFlags(a == 0, false, true, false);
            cycles = 4;
            break;
        }
        case 0xE6: { // AND d8
            uint8_t value = fetchByte();
            a &= value;
            setFlags(a == 0, false, true, false);
            cycles = 8;
            break;
        }
        // --- CONDITIONAL CALLS ---
        case 0xC4: { // CALL NZ, a16
            uint16_t targetAddress = fetchWord();
            if ((f & 0x80) == 0) { // If Zero flag (bit 7) is OFF
                sp -= 2;
                memory.write(sp, pc & 0xFF);
                memory.write(sp + 1, (pc >> 8) & 0xFF);
                pc = targetAddress;
                cycles = 24; // Takes longer if the call happens
            } else {
                cycles = 12; // Faster if it ignores the call
            }
            break;
        }

        case 0xCC: { // CALL Z, a16
            uint16_t targetAddress = fetchWord();
            if ((f & 0x80) != 0) { // If Zero flag (bit 7) is ON
                sp -= 2;
                memory.write(sp, pc & 0xFF);
                memory.write(sp + 1, (pc >> 8) & 0xFF);
                pc = targetAddress;
                cycles = 24;
            } else {
                cycles = 12;
            }
            break;
        }

        case 0xD4: { // CALL NC, a16
            uint16_t targetAddress = fetchWord();
            if ((f & 0x10) == 0) { // If Carry flag (bit 4) is OFF
                sp -= 2;
                memory.write(sp, pc & 0xFF);
                memory.write(sp + 1, (pc >> 8) & 0xFF);
                pc = targetAddress;
                cycles = 24;
            } else {
                cycles = 12;
            }
            break;
        }

        case 0xDC: { // CALL C, a16
            uint16_t targetAddress = fetchWord();
            if ((f & 0x10) != 0) { // If Carry flag (bit 4) is ON
                sp -= 2;
                memory.write(sp, pc & 0xFF);
                memory.write(sp + 1, (pc >> 8) & 0xFF);
                pc = targetAddress;
                cycles = 24;
            } else {
                cycles = 12;
            }
            break;
        }
        // --- 8-BIT IMMEDIATE LOADS ---
        case 0x06: { // LD B, d8
            b = fetchByte();
            cycles = 8;
            break;
        }
        case 0x0E: { // LD C, d8
            c = fetchByte();
            cycles = 8;
            break;
        }
        case 0x16: { // LD D, d8
            d = fetchByte();
            cycles = 8;
            break;
        }
        case 0x1E: { // LD E, d8
            e = fetchByte();
            cycles = 8;
            break;
        }
        case 0x26: { // LD H, d8
            h = fetchByte();
            cycles = 8;
            break;
        }
        case 0x2E: { // LD L, d8
            l = fetchByte();
            cycles = 8;
            break;
        }
        case 0x36: { // LD (HL), d8
            uint8_t value = fetchByte();
            memory.write(hl, value);
            cycles = 12; // Takes 12 cycles because it has to write to memory
            break;
        }
        // --- 8-BIT WRITES TO MEMORY ADDRESS (HL) ---
        case 0x70: { // LD (HL), B
            memory.write(hl, b);
            cycles = 8;
            break;
        }
        case 0x71: { // LD (HL), C
            memory.write(hl, c);
            cycles = 8;
            break;
        }
        case 0x72: { // LD (HL), D
            memory.write(hl, d);
            cycles = 8;
            break;
        }
        case 0x73: { // LD (HL), E
            memory.write(hl, e);
            cycles = 8;
            break;
        }
        case 0x74: { // LD (HL), H
            memory.write(hl, h);
            cycles = 8;
            break;
        }
        case 0x75: { // LD (HL), L
            memory.write(hl, l);
            cycles = 8;
            break;
        }
        // Note: 0x76 is the HALT instruction, so we skip it in this batch!
        case 0x77: { // LD (HL), A
            memory.write(hl, a);
            cycles = 8;
            break;
        }
        // --- 8-BIT INCREMENTS ---
        // Note: 8-bit increments update Z, N, and H, but LEAVE THE CARRY FLAG ALONE!
        case 0x04: { // INC B
            bool hFlag = (b & 0x0F) == 0x0F;
            b++;
            setFlags(b == 0, false, hFlag, (f & 0x10) != 0);
            cycles = 4;
            break;
        }
        case 0x0C: { // INC C
            bool hFlag = (c & 0x0F) == 0x0F;
            c++;
            setFlags(c == 0, false, hFlag, (f & 0x10) != 0);
            cycles = 4;
            break;
        }
        case 0x14: { // INC D
            bool hFlag = (d & 0x0F) == 0x0F;
            d++;
            setFlags(d == 0, false, hFlag, (f & 0x10) != 0);
            cycles = 4;
            break;
        }
        case 0x1C: { // INC E
            bool hFlag = (e & 0x0F) == 0x0F;
            e++;
            setFlags(e == 0, false, hFlag, (f & 0x10) != 0);
            cycles = 4;
            break;
        }
        case 0x24: { // INC H
            bool hFlag = (h & 0x0F) == 0x0F;
            h++;
            setFlags(h == 0, false, hFlag, (f & 0x10) != 0);
            cycles = 4;
            break;
        }
        case 0x2C: { // INC L
            bool hFlag = (l & 0x0F) == 0x0F;
            l++;
            setFlags(l == 0, false, hFlag, (f & 0x10) != 0);
            cycles = 4;
            break;
        }
        case 0x34: { // INC (HL)
            uint8_t value = memory.read(hl);
            bool hFlag = (value & 0x0F) == 0x0F;
            value++;
            memory.write(hl, value);
            setFlags(value == 0, false, hFlag, (f & 0x10) != 0);
            cycles = 12; // 12 cycles because it reads from and writes back to memory
            break;
        }
        case 0x3C: { // INC A
            bool hFlag = (a & 0x0F) == 0x0F;
            a++;
            setFlags(a == 0, false, hFlag, (f & 0x10) != 0);
            cycles = 4;
            break;
        }
        // --- 8-BIT DECREMENTS ---
        // Note: 8-bit decrements update Z, N (set to 1), and H, but LEAVE THE CARRY FLAG ALONE!
        case 0x05: { // DEC B
            bool hFlag = (b & 0x0F) == 0x00; 
            b--;
            setFlags(b == 0, true, hFlag, (f & 0x10) != 0);
            cycles = 4;
            break;
        }
        case 0x0D: { // DEC C
            bool hFlag = (c & 0x0F) == 0x00;
            c--;
            setFlags(c == 0, true, hFlag, (f & 0x10) != 0);
            cycles = 4;
            break;
        }
        case 0x15: { // DEC D
            bool hFlag = (d & 0x0F) == 0x00;
            d--;
            setFlags(d == 0, true, hFlag, (f & 0x10) != 0);
            cycles = 4;
            break;
        }
        case 0x1D: { // DEC E
            bool hFlag = (e & 0x0F) == 0x00;
            e--;
            setFlags(e == 0, true, hFlag, (f & 0x10) != 0);
            cycles = 4;
            break;
        }
        case 0x25: { // DEC H
            bool hFlag = (h & 0x0F) == 0x00;
            h--;
            setFlags(h == 0, true, hFlag, (f & 0x10) != 0);
            cycles = 4;
            break;
        }
        case 0x2D: { // DEC L
            bool hFlag = (l & 0x0F) == 0x00;
            l--;
            setFlags(l == 0, true, hFlag, (f & 0x10) != 0);
            cycles = 4;
            break;
        }
        case 0x35: { // DEC (HL)
            uint8_t value = memory.read(hl);
            bool hFlag = (value & 0x0F) == 0x00;
            value--;
            memory.write(hl, value);
            setFlags(value == 0, true, hFlag, (f & 0x10) != 0);
            cycles = 12; // 12 cycles because it reads from and writes back to memory
            break;
        }
        case 0x3D: { // DEC A
            bool hFlag = (a & 0x0F) == 0x00;
            a--;
            setFlags(a == 0, true, hFlag, (f & 0x10) != 0);
            cycles = 4;
            break;
        }
        // --- INDIRECT MEMORY LOADS/WRITES (BC & DE POINTERS) ---
        case 0x02: { // LD (BC), A
            // Write the value in A to the memory address stored in BC
            memory.write(bc, a);
            cycles = 8;
            break;
        }
        case 0x12: { // LD (DE), A
            // Write the value in A to the memory address stored in DE
            memory.write(de, a);
            cycles = 8;
            break;
        }
        case 0x0A: { // LD A, (BC)
            // Read the value from the memory address stored in BC into A
            a = memory.read(bc);
            cycles = 8;
            break;
        }
        case 0x1A: { // LD A, (DE)
            // Read the value from the memory address stored in DE into A
            a = memory.read(de);
            cycles = 8;
            break;
        }
        
        // --- PREFIX CB ---
        case 0xCB: { 
            uint8_t cbOpcode = fetchByte();
            cycles = executeCB(cbOpcode);
            break;
        }
        
        case 0xEE: { // XOR d8
            uint8_t value = fetchByte();
            a ^= value;
            setFlags(a == 0, false, false, false);
            cycles = 8;
            break;
        }
        // --- 8-BIT ALU: ADDITION ---
        case 0x80: { // ADD A, B
            bool hFlag = (a & 0x0F) + (b & 0x0F) > 0x0F;
            bool cFlag = (uint16_t)a + b > 0xFF;
            a += b;
            setFlags(a == 0, false, hFlag, cFlag);
            cycles = 4;
            break;
        }
        case 0x81: { // ADD A, C
            bool hFlag = (a & 0x0F) + (c & 0x0F) > 0x0F;
            bool cFlag = (uint16_t)a + c > 0xFF;
            a += c;
            setFlags(a == 0, false, hFlag, cFlag);
            cycles = 4;
            break;
        }
        case 0x82: { // ADD A, D
            bool hFlag = (a & 0x0F) + (d & 0x0F) > 0x0F;
            bool cFlag = (uint16_t)a + d > 0xFF;
            a += d;
            setFlags(a == 0, false, hFlag, cFlag);
            cycles = 4;
            break;
        }
        case 0x83: { // ADD A, E
            bool hFlag = (a & 0x0F) + (e & 0x0F) > 0x0F;
            bool cFlag = (uint16_t)a + e > 0xFF;
            a += e;
            setFlags(a == 0, false, hFlag, cFlag);
            cycles = 4;
            break;
        }
        case 0x84: { // ADD A, H
            bool hFlag = (a & 0x0F) + (h & 0x0F) > 0x0F;
            bool cFlag = (uint16_t)a + h > 0xFF;
            a += h;
            setFlags(a == 0, false, hFlag, cFlag);
            cycles = 4;
            break;
        }
        case 0x85: { // ADD A, L
            bool hFlag = (a & 0x0F) + (l & 0x0F) > 0x0F;
            bool cFlag = (uint16_t)a + l > 0xFF;
            a += l;
            setFlags(a == 0, false, hFlag, cFlag);
            cycles = 4;
            break;
        }
        case 0x86: { // ADD A, (HL)
            uint8_t value = memory.read(hl);
            bool hFlag = (a & 0x0F) + (value & 0x0F) > 0x0F;
            bool cFlag = (uint16_t)a + value > 0xFF;
            a += value;
            setFlags(a == 0, false, hFlag, cFlag);
            cycles = 8; // Slower because of memory read
            break;
        }
        case 0x87: { // ADD A, A
            bool hFlag = (a & 0x0F) + (a & 0x0F) > 0x0F;
            bool cFlag = (uint16_t)a + a > 0xFF;
            a += a; // Functionally multiplies A by 2!
            setFlags(a == 0, false, hFlag, cFlag);
            cycles = 4;
            break;
        }
        case 0xC6: { // ADD A, d8
            uint8_t value = fetchByte();
            bool hFlag = (a & 0x0F) + (value & 0x0F) > 0x0F;
            bool cFlag = (uint16_t)a + value > 0xFF;
            a += value;
            setFlags(a == 0, false, hFlag, cFlag);
            cycles = 8;
            break;
        }
        // --- 8-BIT ALU: SUBTRACTION ---
        case 0x90: { // SUB B
            bool hFlag = ((a & 0x0F) - (b & 0x0F)) < 0;
            bool cFlag = a < b;
            a -= b;
            setFlags(a == 0, true, hFlag, cFlag);
            cycles = 4;
            break;
        }
        case 0x91: { // SUB C
            bool hFlag = ((a & 0x0F) - (c & 0x0F)) < 0;
            bool cFlag = a < c;
            a -= c;
            setFlags(a == 0, true, hFlag, cFlag);
            cycles = 4;
            break;
        }
        case 0x92: { // SUB D
            bool hFlag = ((a & 0x0F) - (d & 0x0F)) < 0;
            bool cFlag = a < d;
            a -= d;
            setFlags(a == 0, true, hFlag, cFlag);
            cycles = 4;
            break;
        }
        case 0x93: { // SUB E
            bool hFlag = ((a & 0x0F) - (e & 0x0F)) < 0;
            bool cFlag = a < e;
            a -= e;
            setFlags(a == 0, true, hFlag, cFlag);
            cycles = 4;
            break;
        }
        case 0x94: { // SUB H
            bool hFlag = ((a & 0x0F) - (h & 0x0F)) < 0;
            bool cFlag = a < h;
            a -= h;
            setFlags(a == 0, true, hFlag, cFlag);
            cycles = 4;
            break;
        }
        case 0x95: { // SUB L
            bool hFlag = ((a & 0x0F) - (l & 0x0F)) < 0;
            bool cFlag = a < l;
            a -= l;
            setFlags(a == 0, true, hFlag, cFlag);
            cycles = 4;
            break;
        }
        case 0x96: { // SUB (HL)
            uint8_t value = memory.read(hl);
            bool hFlag = ((a & 0x0F) - (value & 0x0F)) < 0;
            bool cFlag = a < value;
            a -= value;
            setFlags(a == 0, true, hFlag, cFlag);
            cycles = 8;
            break;
        }
        case 0x97: { // SUB A
            // Subtracting A from A always equals 0!
            a -= a; 
            setFlags(true, true, false, false);
            cycles = 4;
            break;
        }
        case 0xD6: { // SUB d8
            uint8_t value = fetchByte();
            bool hFlag = ((a & 0x0F) - (value & 0x0F)) < 0;
            bool cFlag = a < value;
            a -= value;
            setFlags(a == 0, true, hFlag, cFlag);
            cycles = 8;
            break;
        }
        // --- 8-BIT LOADS FROM MEMORY ADDRESS (HL) ---
        case 0x46: { // LD B, (HL)
            b = memory.read(hl);
            cycles = 8;
            break;
        }
        case 0x4E: { // LD C, (HL)
            c = memory.read(hl);
            cycles = 8;
            break;
        }
        case 0x56: { // LD D, (HL)
            d = memory.read(hl);
            cycles = 8;
            break;
        }
        case 0x5E: { // LD E, (HL)
            e = memory.read(hl);
            cycles = 8;
            break;
        }
        case 0x66: { // LD H, (HL)
            h = memory.read(hl);
            cycles = 8;
            break;
        }
        case 0x6E: { // LD L, (HL)
            l = memory.read(hl);
            cycles = 8;
            break;
        }
        // --- ACCUMULATOR ROTATIONS ---
        // Note: These specific operations ALWAYS set the Z, N, and H flags to 0!
        
        case 0x07: { // RLCA (Rotate Left Circular A)
            bool newCarry = (a & 0x80) != 0; // Check if bit 7 is 1
            a = (a << 1) | (newCarry ? 0x01 : 0x00); // Shift left, wrap bit 7 to bit 0
            setFlags(false, false, false, newCarry);
            cycles = 4;
            break;
        }
        
        case 0x0F: { // RRCA (Rotate Right Circular A)
            bool newCarry = (a & 0x01) != 0; // Check if bit 0 is 1
            a = (a >> 1) | (newCarry ? 0x80 : 0x00); // Shift right, wrap bit 0 to bit 7
            setFlags(false, false, false, newCarry);
            cycles = 4;
            break;
        }
        
        case 0x17: { // RLA (Rotate Left A through Carry)
            bool oldCarry = (f & 0x10) != 0; // Read the current Carry flag
            bool newCarry = (a & 0x80) != 0; // Check if bit 7 is 1
            a = (a << 1) | (oldCarry ? 0x01 : 0x00); // Shift left, put old Carry into bit 0
            setFlags(false, false, false, newCarry);
            cycles = 4;
            break;
        }
        
        case 0x1F: { // RRA (Rotate Right A through Carry)
            bool oldCarry = (f & 0x10) != 0; // Read the current Carry flag
            bool newCarry = (a & 0x01) != 0; // Check if bit 0 is 1
            a = (a >> 1) | (oldCarry ? 0x80 : 0x00); // Shift right, put old Carry into bit 7
            setFlags(false, false, false, newCarry);
            cycles = 4;
            break;
        }
        // --- 8-BIT LOADS FROM ACCUMULATOR (A) TO REGISTERS ---
        case 0x47: { // LD B, A
            b = a;
            cycles = 4;
            break;
        }
        case 0x4F: { // LD C, A
            c = a;
            cycles = 4;
            break;
        }
        case 0x57: { // LD D, A
            d = a;
            cycles = 4;
            break;
        }
        case 0x5F: { // LD E, A
            e = a;
            cycles = 4;
            break;
        }
        case 0x67: { // LD H, A
            h = a;
            cycles = 4;
            break;
        }
        case 0x6F: { // LD L, A
            l = a;
            cycles = 4;
            break;
        }
        // --- 8-BIT ALU: ADD WITH CARRY (ADC) ---
        case 0x88: { uint8_t carry = (f & 0x10) ? 1 : 0; bool h = (a & 0x0F) + (b & 0x0F) + carry > 0x0F; bool c_flag = (int)a + b + carry > 0xFF; a = a + b + carry; setFlags(a == 0, false, h, c_flag); cycles = 4; break; }
        case 0x89: { uint8_t carry = (f & 0x10) ? 1 : 0; bool h = (a & 0x0F) + (c & 0x0F) + carry > 0x0F; bool c_flag = (int)a + c + carry > 0xFF; a = a + c + carry; setFlags(a == 0, false, h, c_flag); cycles = 4; break; }
        case 0x8A: { uint8_t carry = (f & 0x10) ? 1 : 0; bool h = (a & 0x0F) + (d & 0x0F) + carry > 0x0F; bool c_flag = (int)a + d + carry > 0xFF; a = a + d + carry; setFlags(a == 0, false, h, c_flag); cycles = 4; break; }
        case 0x8B: { uint8_t carry = (f & 0x10) ? 1 : 0; bool h = (a & 0x0F) + (e & 0x0F) + carry > 0x0F; bool c_flag = (int)a + e + carry > 0xFF; a = a + e + carry; setFlags(a == 0, false, h, c_flag); cycles = 4; break; }
        case 0x8C: { uint8_t carry = (f & 0x10) ? 1 : 0; bool h = (a & 0x0F) + (h & 0x0F) + carry > 0x0F; bool c_flag = (int)a + h + carry > 0xFF; a = a + h + carry; setFlags(a == 0, false, h, c_flag); cycles = 4; break; }
        case 0x8D: { uint8_t carry = (f & 0x10) ? 1 : 0; bool h = (a & 0x0F) + (l & 0x0F) + carry > 0x0F; bool c_flag = (int)a + l + carry > 0xFF; a = a + l + carry; setFlags(a == 0, false, h, c_flag); cycles = 4; break; }
        case 0x8E: { uint8_t val = memory.read(hl); uint8_t carry = (f & 0x10) ? 1 : 0; bool h = (a & 0x0F) + (val & 0x0F) + carry > 0x0F; bool c_flag = (int)a + val + carry > 0xFF; a = a + val + carry; setFlags(a == 0, false, h, c_flag); cycles = 8; break; }
        case 0x8F: { uint8_t carry = (f & 0x10) ? 1 : 0; bool h = (a & 0x0F) + (a & 0x0F) + carry > 0x0F; bool c_flag = (int)a + a + carry > 0xFF; a = a + a + carry; setFlags(a == 0, false, h, c_flag); cycles = 4; break; }
        case 0xCE: { // ADC A, d8
            uint8_t val = fetchByte();
            uint8_t carry = (f & 0x10) ? 1 : 0;
            bool h = (a & 0x0F) + (val & 0x0F) + carry > 0x0F;
            bool c_flag = (int)a + val + carry > 0xFF;
            a = a + val + carry;
            setFlags(a == 0, false, h, c_flag);
            cycles = 8;
            break;
        }
        // --- CONDITIONAL RETURNS ---
        case 0xC0: { // RET NZ (Return if Not Zero)
            if ((f & 0x80) == 0) {
                uint16_t low = memory.read(sp++);
                uint16_t high = memory.read(sp++);
                pc = (high << 8) | low;
                cycles = 20; // 20 cycles if taken
            } else {
                cycles = 8;  // 8 cycles if not taken
            }
            break;
        }
        case 0xC8: { // RET Z (Return if Zero)
            if ((f & 0x80) != 0) {
                uint16_t low = memory.read(sp++);
                uint16_t high = memory.read(sp++);
                pc = (high << 8) | low;
                cycles = 20;
            } else {
                cycles = 8;
            }
            break;
        }
        case 0xD0: { // RET NC (Return if No Carry)
            if ((f & 0x10) == 0) {
                uint16_t low = memory.read(sp++);
                uint16_t high = memory.read(sp++);
                pc = (high << 8) | low;
                cycles = 20;
            } else {
                cycles = 8;
            }
            break;
        }
        case 0xD8: { // RET C (Return if Carry)
            if ((f & 0x10) != 0) {
                uint16_t low = memory.read(sp++);
                uint16_t high = memory.read(sp++);
                pc = (high << 8) | low;
                cycles = 20;
            } else {
                cycles = 8;
            }
            break;
        }
        // --- 16-BIT ALU: ADD TO HL ---
        // Note: 16-bit ADD does NOT modify the Zero (Z) flag!
        case 0x09: { // ADD HL, BC
            uint32_t result = hl + bc;
            bool h = ((hl & 0x0FFF) + (bc & 0x0FFF)) > 0x0FFF;
            hl = (uint16_t)result;
            setFlags((f & 0x80) != 0, false, h, result > 0xFFFF);
            cycles = 8;
            break;
        }
        case 0x19: { // ADD HL, DE
            uint32_t result = hl + de;
            bool h = ((hl & 0x0FFF) + (de & 0x0FFF)) > 0x0FFF;
            hl = (uint16_t)result;
            setFlags((f & 0x80) != 0, false, h, result > 0xFFFF);
            cycles = 8;
            break;
        }
        case 0x29: { // ADD HL, HL
            uint32_t result = hl + hl;
            bool h = ((hl & 0x0FFF) + (hl & 0x0FFF)) > 0x0FFF;
            hl = (uint16_t)result;
            setFlags((f & 0x80) != 0, false, h, result > 0xFFFF);
            cycles = 8;
            break;
        }
        case 0x39: { // ADD HL, SP
            uint32_t result = hl + sp;
            bool h = ((hl & 0x0FFF) + (sp & 0x0FFF)) > 0x0FFF;
            hl = (uint16_t)result;
            setFlags((f & 0x80) != 0, false, h, result > 0xFFFF);
            cycles = 8;
            break;
        }
        case 0xE9: { // JP (HL)
            // Jump to the address currently held in the HL register
            pc = hl;
            
            cycles = 4; // This is a very fast internal transfer
            break;
        }
        case 0xF6: { // OR d8
            // Fetch the immediate 8-bit value from memory
            uint8_t value = fetchByte();
            
            // Perform bitwise OR and store in Accumulator
            a |= value;
            
            // Update flags: Z is evaluated, N, H, and C are entirely cleared
            setFlags(a == 0, false, false, false);
            
            cycles = 8; // Takes 8 clock cycles to fetch and execute
            break;
        }
        // --- CONDITIONAL ABSOLUTE JUMPS ---
        case 0xC2: { // JP NZ, a16 (Jump if Not Zero)
            uint16_t targetAddress = fetchWord();
            if ((f & 0x80) == 0) { // If Zero flag (bit 7) is OFF
                pc = targetAddress;
                cycles = 16; // Takes 16 cycles if the jump happens
            } else {
                cycles = 12; // Takes 12 cycles if the jump is skipped
            }
            break;
        }

        case 0xCA: { // JP Z, a16 (Jump if Zero)
            uint16_t targetAddress = fetchWord();
            if ((f & 0x80) != 0) { // If Zero flag (bit 7) is ON
                pc = targetAddress;
                cycles = 16;
            } else {
                cycles = 12;
            }
            break;
        }

        case 0xD2: { // JP NC, a16 (Jump if No Carry)
            uint16_t targetAddress = fetchWord();
            if ((f & 0x10) == 0) { // If Carry flag (bit 4) is OFF
                pc = targetAddress;
                cycles = 16;
            } else {
                cycles = 12;
            }
            break;
        }

        case 0xDA: { // JP C, a16 (Jump if Carry)
            uint16_t targetAddress = fetchWord();
            if ((f & 0x10) != 0) { // If Carry flag (bit 4) is ON
                pc = targetAddress;
                cycles = 16;
            } else {
                cycles = 12;
            }
            break;
        }
        // --- STACK POINTER OFFSET OPERATIONS ---
        // Note: These instructions treat the signed offset as an unsigned byte for 
        // H/C flag evaluation, and both explicitly force the Zero (Z) flag to 0!
        case 0xE8: { // ADD SP, r8
            int8_t offset = (int8_t)fetchByte();
            uint8_t unsignedOffset = (uint8_t)offset;
            
            // Calculate flags using the lower 8 bits of SP
            bool hFlag = ((sp & 0x0F) + (unsignedOffset & 0x0F)) > 0x0F;
            bool cFlag = ((sp & 0xFF) + (unsignedOffset & 0xFF)) > 0xFF;
            
            sp += offset;
            setFlags(false, false, hFlag, cFlag);
            cycles = 16;
            break;
        }

        case 0xF8: { // LD HL, SP+r8
            int8_t offset = (int8_t)fetchByte();
            uint8_t unsignedOffset = (uint8_t)offset;
            
            // Calculate flags using the lower 8 bits of SP
            bool hFlag = ((sp & 0x0F) + (unsignedOffset & 0x0F)) > 0x0F;
            bool cFlag = ((sp & 0xFF) + (unsignedOffset & 0xFF)) > 0xFF;
            
            hl = sp + offset;
            setFlags(false, false, hFlag, cFlag);
            cycles = 12;
            break;
        }
        // --- 8-BIT REGISTER-TO-REGISTER LOADS ---
        // Destination B
        case 0x40: { b = b; cycles = 4; break; } // LD B, B
        case 0x41: { b = c; cycles = 4; break; } // LD B, C
        case 0x42: { b = d; cycles = 4; break; } // LD B, D
        case 0x43: { b = e; cycles = 4; break; } // LD B, E
        case 0x44: { b = h; cycles = 4; break; } // LD B, H
        case 0x45: { b = l; cycles = 4; break; } // LD B, L
        // (0x46 and 0x47 are already implemented!)

        // Destination C
        case 0x48: { c = b; cycles = 4; break; } // LD C, B
        case 0x49: { c = c; cycles = 4; break; } // LD C, C
        case 0x4A: { c = d; cycles = 4; break; } // LD C, D
        case 0x4B: { c = e; cycles = 4; break; } // LD C, E
        case 0x4C: { c = h; cycles = 4; break; } // LD C, H
        case 0x4D: { c = l; cycles = 4; break; } // LD C, L
        // (0x4E and 0x4F are already implemented!)

        // Destination D
        case 0x50: { d = b; cycles = 4; break; } // LD D, B
        case 0x51: { d = c; cycles = 4; break; } // LD D, C
        case 0x52: { d = d; cycles = 4; break; } // LD D, D
        case 0x53: { d = e; cycles = 4; break; } // LD D, E
        case 0x54: { d = h; cycles = 4; break; } // LD D, H
        case 0x55: { d = l; cycles = 4; break; } // LD D, L
        // (0x56 and 0x57 are already implemented!)

        // Destination E
        case 0x58: { e = b; cycles = 4; break; } // LD E, B
        case 0x59: { e = c; cycles = 4; break; } // LD E, C
        case 0x5A: { e = d; cycles = 4; break; } // LD E, D
        case 0x5B: { e = e; cycles = 4; break; } // LD E, E
        case 0x5C: { e = h; cycles = 4; break; } // LD E, H
        case 0x5D: { e = l; cycles = 4; break; } // LD E, L
        // (0x5E and 0x5F are already implemented!)

        // Destination H
        case 0x60: { h = b; cycles = 4; break; } // LD H, B
        case 0x61: { h = c; cycles = 4; break; } // LD H, C
        case 0x62: { h = d; cycles = 4; break; } // LD H, D
        case 0x63: { h = e; cycles = 4; break; } // LD H, E
        case 0x64: { h = h; cycles = 4; break; } // LD H, H
        case 0x65: { h = l; cycles = 4; break; } // LD H, L
        // (0x66 and 0x67 are already implemented!)

        // Destination L
        case 0x68: { l = b; cycles = 4; break; } // LD L, B
        case 0x69: { l = c; cycles = 4; break; } // LD L, C
        case 0x6A: { l = d; cycles = 4; break; } // LD L, D
        case 0x6B: { l = e; cycles = 4; break; } // LD L, E
        case 0x6C: { l = h; cycles = 4; break; } // LD L, H
        case 0x6D: { l = l; cycles = 4; break; } // LD L, L
        case 0x27: { // DAA (Decimal Adjust Accumulator)
            bool nFlag = (f & 0x40) != 0; // Bit 6: Subtract Flag
            bool hFlag = (f & 0x20) != 0; // Bit 5: Half-Carry Flag
            bool cFlag = (f & 0x10) != 0; // Bit 4: Carry Flag

            if (!nFlag) { // --- AFTER AN ADDITION ---
                if (cFlag || a > 0x99) {
                    a += 0x60;
                    cFlag = true;
                }
                if (hFlag || (a & 0x0F) > 0x09) {
                    a += 0x06;
                }
            } else { // --- AFTER A SUBTRACTION ---
                if (cFlag) {
                    a -= 0x60;
                }
                if (hFlag) {
                    a -= 0x06;
                }
            }

            // Flags: Z is updated, N is preserved, H is ALWAYS cleared, C is updated
            setFlags(a == 0, nFlag, false, cFlag);
            cycles = 4;
            break;
        }
        case 0xF9: { // LD SP, HL
            // Copy the 16-bit value from HL directly into the Stack Pointer
            sp = hl;
            
            cycles = 8; // Takes 8 clock cycles
            break;
        }
        case 0x76: { // HALT
            // Put the CPU into a halted state waiting for an interrupt
            halted = true;
            
            cycles = 4;
            break;
        }
        case 0x10: { // STOP
            // CRUCIAL: STOP is a 2-byte instruction. 
            // We must fetch the dummy byte following it to maintain PC alignment.
            uint8_t dummy = fetchByte();
            
            // Put the CPU into a stopped state
            // (If supporting GBC later, check for speed switch logic here via 0xFF4D)
            stopped = true; 
            
            cycles = 4;
            break;
        }
        case 0x08: { // LD (a16), SP
            // Fetch the 16-bit destination target address
            uint16_t address = fetchWord();
            
            // Write the 16-bit Stack Pointer to memory (Little-Endian format)
            memory.write(address, sp & 0xFF);         // Lower byte
            memory.write(address + 1, (sp >> 8) & 0xFF); // Upper byte
            
            cycles = 20; // 20 clock cycles (loads 3 bytes total, performs 2 memory writes)
            break;
        }
        
        // (0x6E and 0x6F are already implemented!)
        // (Note: You already wrote 0x77 for LD (HL), A and 0x7F for LD A, A!)
        // (Note: You already wrote 0x7E for LD A, (HL) in a previous batch!)
        default:
            printf("Unimplemented opcode: 0x%02X at PC: 0x%04X\n", opcode, pc - 1);
            exit(1); 
            break;
    }

    return cycles;
}

void CPU::requestInterrupt(int bit){
    uint8_t req = memory.read(0xFF0F);
    req |= (1 << bit);
    memory.write(0xFF0F, req);
}

void CPU::handleInterrupts(){
    if (ime){
        uint8_t req = memory.read(0xFF0F);     
        uint8_t enable = memory.read(0xFFFF);  
        
        if (req & enable){
            for (int i = 0; i < 5; i++) {
                if ((req & (1 << i)) && (enable & (1 << i))){
                    ime = false; 
                    
                    req &= ~(1 << i);
                    memory.write(0xFF0F, req);
                    
                    sp -= 2;
                    memory.write(sp, pc & 0xFF);         
                    memory.write(sp + 1, (pc >> 8) & 0xFF); 
                    
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

    // 4. Write the modified value back
    if (operation != 1) {
        if (regIndex == 6) {
            memory.write(hl, value);
        } else {
            *targetReg = value;
        }
    }

    return cycles;
}