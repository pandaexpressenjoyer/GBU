#include "Memory.h"
#include <cstdio>

using namespace std;

// Initialize the timer by passing a reference to the memory bus (*this)
Memory::Memory(Controller& ctrl) : controller(ctrl), timer(*this) {
    data.fill(0);
}

bool Memory::load(const string& path){
    return cart.load(path);
}

uint8_t Memory::read(uint16_t address) const{
    if (address < 0x8000) return cart.read(address);
    if (address >= 0xA000 && address <= 0xBFFF) return cart.read(address);
    if (address >= 0xFF00) return readIO(address);
    return data[address];
}

void Memory::write(uint16_t address, uint8_t value){
    if (address < 0x8000) { cart.write(address, value); return; }
    if (address >= 0xA000 && address <= 0xBFFF) { cart.write(address, value); return; }
    if (address >= 0xFF00) { writeIO(address, value); return; }
    data[address] = value;
}

uint8_t Memory::readIO(uint16_t address) const{
    // Route Timer Registers
    if (address >= 0xFF04 && address <= 0xFF07) return timer.read(address); 
    
    // Route Joypad Register
    if (address == 0xFF00) return controller.read(); 
    
    return data[address];
}

void Memory::writeIO(uint16_t address, uint8_t value){
    // Route Timer Registers
    if (address >= 0xFF04 && address <= 0xFF07) { timer.write(address, value); return; }
    
    if (address == 0xFF01) { printf("%c", value); }
    
    // Route Joypad Register
    if (address == 0xFF00) { controller.write(value); return; } 
    
    data[address] = value;
}