#include "Cartridge.h"
#include <fstream>
#include <iostream>
#include <iomanip>

using namespace std;

Cartridge::Cartridge() : cartridgeType(0), romSizeCode(0), ramSizeCode(0),
                         hasMBC1(false), hasBattery(false), ramEnabled(false),
                         romBank(1), ramBank(0), bankingMode(0) {}

// The destructor runs automatically when the emulator closes
Cartridge::~Cartridge() {
    if (hasBattery && ramData.size() > 0) {
        saveGame();
    }
}

bool Cartridge::load(const string& path) {
    ifstream file(path, ios::binary | ios::ate);
    if (!file) {
        cerr << "Failed to open ROM: " << path << endl;
        return false;
    }

    streamsize size = file.tellg();
    file.seekg(0, ios::beg);

    romData.resize(size);
    if (!file.read(reinterpret_cast<char*>(romData.data()), size)) {
        cerr << "Failed to read ROM data!" << endl;
        return false;
    }

    // Generate the save file path (e.g. "pokemon.gb" -> "pokemon.sav")
    size_t lastDot = path.find_last_of(".");
    if (lastDot != string::npos) {
        saveFilePath = path.substr(0, lastDot) + ".sav";
    } else {
        saveFilePath = path + ".sav";
    }

    parseHeader();
    
    // Allocate exact RAM needed based on the cartridge header
    int ramSizes[] = {0, 2048, 8192, 32768, 131072, 65536};
    if (ramSizeCode < 6) {
        ramData.resize(ramSizes[ramSizeCode], 0);
    }
    
    // If it has a battery, attempt to load the .sav file from your hard drive
    if (hasBattery && ramData.size() > 0) {
        loadSaveFile();
    }

    return true;
}

void Cartridge::parseHeader() {
    title = "";
    for (int i = 0x0134; i <= 0x0143; i++) {
        if (romData[i] == 0) break; // Titles are null-terminated
        title += static_cast<char>(romData[i]);
    }

    cartridgeType = romData[0x0147];
    romSizeCode = romData[0x0148];
    ramSizeCode = romData[0x0149];

    // Flag the chip features
    if (cartridgeType == 0x01 || cartridgeType == 0x02 || cartridgeType == 0x03) {
        hasMBC1 = true;
    }
    if (cartridgeType == 0x03 || cartridgeType == 0x0F || cartridgeType == 0x13 || cartridgeType == 0x1B) {
        hasBattery = true;
    }

    cout << "--- CARTRIDGE LOADED ---" << endl;
    cout << "Title:    " << title << endl;
    cout << "Type:     " << getTypeName() << " (0x" << hex << uppercase << setw(2) << setfill('0') << (int)cartridgeType << ")" << dec << endl;
    cout << "ROM Size: " << (32 << romSizeCode) << " KB" << endl;
    cout << "RAM Size: " << ramData.size() << " Bytes" << endl;
    cout << "------------------------" << endl;
}

uint8_t Cartridge::read(uint16_t address) const {
    // ROM Bank 00 (0x0000 - 0x3FFF) Always mapped to the first 16KB
    if (address < 0x4000) {
        return romData[address];
    }
    
    // ROM Bank 01-7F (0x4000 - 0x7FFF) Dynamically mapped
    if (address >= 0x4000 && address < 0x8000) {
        uint32_t mappedAddress = (romBank * 0x4000) + (address - 0x4000);
        if (mappedAddress < romData.size()) {
            return romData[mappedAddress];
        }
        return 0xFF; // Out of bounds fallback
    }
    
    // External RAM Area (0xA000 - 0xBFFF) Dynamically mapped
    if (address >= 0xA000 && address <= 0xBFFF) {
        if (ramEnabled && ramData.size() > 0) {
            uint32_t mappedAddress = (ramBank * 0x2000) + (address - 0xA000);
            if (mappedAddress < ramData.size()) {
                return ramData[mappedAddress];
            }
        }
        return 0xFF; // RAM disabled or out of bounds
    }
    
    return 0xFF; 
}

void Cartridge::write(uint16_t address, uint8_t value) {
    if (!hasMBC1) return; // If standard ROM, ignore writes

    // 1. RAM Enable (0x0000 - 0x1FFF)
    if (address < 0x2000) {
        ramEnabled = ((value & 0x0F) == 0x0A);
    }
    // 2. ROM Bank Number (0x2000 - 0x3FFF)
    else if (address >= 0x2000 && address < 0x4000) {
        uint8_t lower5 = value & 0x1F;
        if (lower5 == 0) lower5 = 1; // MBC1 Quirk: Bank 0 acts as Bank 1
        
        romBank = (romBank & 0xE0) | lower5;
    }
    // 3. RAM Bank Number / Upper ROM Bank Number (0x4000 - 0x5FFF)
    else if (address >= 0x4000 && address < 0x6000) {
        uint8_t bits = value & 0x03;
        if (bankingMode == 0) {
            // ROM Mode: Adjust upper 2 bits of ROM Bank
            romBank = (romBank & 0x1F) | (bits << 5);
        } else {
            // RAM Mode: Adjust RAM Bank
            ramBank = bits;
        }
    }
    // 4. ROM/RAM Mode Select (0x6000 - 0x7FFF)
    else if (address >= 0x6000 && address < 0x8000) {
        bankingMode = value & 0x01;
        if (bankingMode == 0) {
            ramBank = 0; // Switching back to ROM mode locks RAM to Bank 0
        }
    }
    // 5. External RAM Write (0xA000 - 0xBFFF)
    else if (address >= 0xA000 && address <= 0xBFFF) {
        if (ramEnabled && ramData.size() > 0) {
            uint32_t mappedAddress = (ramBank * 0x2000) + (address - 0xA000);
            if (mappedAddress < ramData.size()) {
                ramData[mappedAddress] = value;
            }
        }
    }
}

const char* Cartridge::getTypeName() const {
    switch (cartridgeType) {
        case 0x00: return "ROM ONLY";
        case 0x01: return "MBC1";
        case 0x02: return "MBC1+RAM";
        case 0x03: return "MBC1+RAM+BATTERY";
        case 0x0F: return "MBC3+TIMER+BATTERY";
        case 0x11: return "MBC3";
        case 0x13: return "MBC3+RAM+BATTERY";
        default:   return "OTHER / UNIMPLEMENTED";
    }
}

void Cartridge::loadSaveFile() {
    ifstream file(saveFilePath, ios::binary);
    if (file) {
        file.read(reinterpret_cast<char*>(ramData.data()), ramData.size());
        cout << "Loaded save file: " << saveFilePath << endl;
    }
}

void Cartridge::saveGame() {
    ofstream file(saveFilePath, ios::binary);
    if (file) {
        file.write(reinterpret_cast<const char*>(ramData.data()), ramData.size());
        cout << "Game saved to: " << saveFilePath << endl;
    }
}