#include <stdio.h>
#include <stdlib.h>

#include "Cartridge.h"

// include all the different memory chips that we might use
#include "MemoryChips/ROMOnly.h"
#include "MemoryChips/MBC1.h"
#include "MemoryChips/MBC3.h"
#include "MemoryChips/MBC5.h"

// constants

// memory offsets into the ROM
const DoubleByte HEADER_START          = 0x100;
const DoubleByte HEADER_CARTRIDGE_TYPE = 0x147;
const DoubleByte HEADER_ROM_SIZE       = 0x148;
const DoubleByte HEADER_RAM_SIZE       = 0x149;
const DoubleByte HEADER_END            = 0x014F;

/* 
    the BIOS contains the instructions that the gameboy uses to boot up. 
    the assembly instructions for the hexadecimal below can be found here:
    https://gbdev.gg8.se/wiki/articles/Gameboy_Bootstrap_ROM#The_DMG_bootstrap
*/
const Byte BIOS[256] = 
{
    0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB, 0x21, 0x26, 0xFF, 0x0E,
    0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3, 0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0,
    0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B,
    0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22, 0x23, 0x05, 0x20, 0xF9,
    0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99, 0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20,
    0xF9, 0x2E, 0x0F, 0x18, 0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04,
    0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90, 0x20, 0xFA, 0x0D, 0x20, 0xF7, 0x1D, 0x20, 0xF2,
    0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62, 0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06,
    0x7B, 0xE2, 0x0C, 0x3E, 0x87, 0xE2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
    0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17, 0xC1, 0xCB, 0x11, 0x17,
    0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9, 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
    0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
    0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
    0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 0x3C, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C,
    0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x00, 0x00, 0x23, 0x7D, 0xFE, 0x34, 0x20,
    0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x00, 0x00, 0x3E, 0x01, 0xE0, 0x50
};

// inserts the BIOS code into the start of the memory
void Cartridge::loadBIOS(Byte* memory)
{
    for (int byte = 0; byte < 256; byte++)
    {
        mReplacedROMData[byte] = memory[byte];
        memory[byte] = BIOS[byte];
    }
}

void Cartridge::unloadBIOS(Byte* memory)
{
    for (int byte = 0; byte < 256; byte++)
        memory[byte] = mReplacedROMData[byte];
}

// load's a ROM into memory
void Cartridge::loadROM(const char* romDir, MMU* mmu)
{
    // open the file provided by the directory as binary
    FILE* romFile = fopen(romDir, "rb");
    if (romFile == NULL)
    {
        printf("ROM file failed to load! Was the directory provided incorrect?");
        exit(-1);
    }

    // find the length of the ROM file
    fseek(romFile, 0L, SEEK_END); // seek to the end of the file
    mROMSize = ftell(romFile);    // get the position of the cursor (now at the end of the file)
    rewind(romFile);              // seek back to the beginning of the file

    mmu->romMemory = new Byte[mROMSize];

    // load the ROM file into memory
    fread(mmu->romMemory, mROMSize, 1, romFile); // fills the ROM's memory with the data provided by the ROM file

    switch (getType(mmu->romMemory))
    {
        case CartridgeType::ROM_ONLY: 
        case CartridgeType::ROM_AND_RAM:
        case CartridgeType::ROM_AND_RAM_AND_BATTERY:
            mmu->memoryChip = new ROMOnly(mmu->romMemory, getNumRamBanks(mmu->romMemory));
            break;

        case CartridgeType::MBC1:
        case CartridgeType::MBC1_AND_RAM:
        case CartridgeType::MBC1_AND_RAM_AND_BATTERY:
            mmu->memoryChip = new MBC1(mmu->romMemory, getNumRomBanks(mmu->romMemory), getNumRamBanks(mmu->romMemory));
            break;

        case CartridgeType::MBC3:
        case CartridgeType::MBC3_AND_RAM:
        case CartridgeType::MBC3_AND_TIMER_AND_BATTERY:
        case CartridgeType::MBC3_AND_TIMER_AND_RAM_AND_BATTERY:
        case CartridgeType::MBC3_AND_RAM_AND_BATTERY:
            mmu->memoryChip = new MBC3(mmu->romMemory, getNumRomBanks(mmu->romMemory), getNumRamBanks(mmu->romMemory));
            break;

        case CartridgeType::MBC5:
        case CartridgeType::MBC5_AND_RAM:
        case CartridgeType::MBC5_AND_RAM_AND_BATTERY:
        case CartridgeType::MBC5_AND_RUMBLE:
        case CartridgeType::MBC5_AND_RUMBLE_AND_RAM:
        case CartridgeType::MBC5_AND_RUMBLE_AND_RAM_AND_BATTERY:
            mmu->memoryChip = new MBC5(mmu->romMemory, getNumRomBanks(mmu->romMemory), getNumRamBanks(mmu->romMemory));
            break;

        default:
            printf("That memory bank/cartridge type is not supported!\n");
            exit(0);
    }
}

CartridgeType Cartridge::getType(Byte* memory)
{
    return (CartridgeType)memory[HEADER_CARTRIDGE_TYPE];
}

int Cartridge::getNumRomBanks(Byte* memory)
{
    switch (memory[HEADER_ROM_SIZE])
    {
        case 0x0: return 2; // no banking
        case 0x1: return 4;
        case 0x2: return 8;
        case 0x3: return 16;
        case 0x4: return 32;
        case 0x5: return 64;
        case 0x6: return 128;
        case 0x7: return 256;
        case 0x8: return 512;
        default: return -1;
    }
}

int Cartridge::getNumRamBanks(Byte* memory)
{
    switch (memory[HEADER_ROM_SIZE])
    {
        case 0x0: return 0; // no RAM
        case 0x2: return 1;
        case 0x3: return 4;
        case 0x4: return 16;
        case 0x5: return 8;
        default:  return 0;
    }   
}
