#include "Memory.h"
#include <Controller.h>
#include <fstream>
#include <cstdio>


using namespace std;

// The Fix: Initializer list binds the 'ctrl' parameter to the 'controller' reference
Memory::Memory(Controller& ctrl) : controller(ctrl) {
    data.fill(0);
    rom.fill(0);
}

bool Memory::load(const string& path){
    ifstream file(path, ios::binary);
    if (!file){
        printf("Failed to open ROM: %s\n", path.c_str());
        return false;
    }
    file.read(reinterpret_cast<char*>(rom.data()), rom.size());
    return true;
}

uint8_t Memory::read(uint16_t address) const{
    if (address < 0x8000)   return rom[address];
    if (address >= 0xFF00)  return readIO(address);
    return data[address];
}

void Memory::write(uint16_t address, uint8_t value){
    if (address < 0x8000)  return;
    if (address >= 0xFF00) { writeIO(address, value); return; }
    data[address] = value;
}

uint8_t Memory::readIO(uint16_t address) const{
    // Once you're ready to wire up the joypad, you'll un-comment this:
    // if (address == 0xFF00) return controller.read();
    
    return data[address];
}

void Memory::writeIO(uint16_t address, uint8_t value){
    if (address == 0xFF01) {
        printf("%c", value);
    }
    
    // Once you're ready to wire up the joypad, you'll un-comment this:
    // if (address == 0xFF00) { controller.write(value); return; }
    
    data[address] = value;
}