#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <string>
#include <vector>
#include <cstdint>

class Cartridge {
public:
    Cartridge();
    ~Cartridge(); // Added Destructor to trigger automatic saving

    bool load(const std::string& path);
    
    uint8_t read(uint16_t address) const;
    void write(uint16_t address, uint8_t value);

    std::string title;
    uint8_t cartridgeType;
    uint8_t romSizeCode;
    uint8_t ramSizeCode;

private:
    std::vector<uint8_t> romData;
    std::vector<uint8_t> ramData; 

    // MBC1 State Variables
    bool hasMBC1;
    bool hasBattery;
    bool ramEnabled;
    uint8_t romBank;
    uint8_t ramBank;
    uint8_t bankingMode; // 0 = ROM Banking Mode, 1 = RAM Banking Mode
    
    std::string saveFilePath;

    void parseHeader();
    const char* getTypeName() const;
    void loadSaveFile();
    void saveGame();
};

#endif