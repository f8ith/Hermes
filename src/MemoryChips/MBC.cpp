#include "MBC.h"

// the offset at which the data for memory bank controllers in the emulator's customly encoded save files starts
const DoubleByte SAVE_FILE_MBC_OFFSET = 0x800C;

MBC::MBC(Byte* memory, DoubleByte numOfRomBanks, DoubleByte numOfRamBanks)
    : MemoryChip(memory)
{
    mSelectedROMBank = 1;
    mSelectedRAMBank = 0;
    mNumOfRomBanks = numOfRomBanks;
    mNumOfRamBanks = numOfRamBanks;

    mRAMEnabled = false;

    mRAMBanks = new Byte*[numOfRamBanks];
    for (int bank = 0; bank < numOfRamBanks; bank++)
        mRAMBanks[bank] = new Byte[RAM_BANK_SIZE];
}

Byte MBC::readByte(DoubleByte addr)
{
    // if trying to read a ROM memory bank that is NOT bank 0:
    if (addr >= 0x4000 && addr <= 0x7FFF)
        return mROMMemory[(addr - 0x4000) + ROM_BANK_SIZE * mSelectedROMBank];
    else if (addr >= 0xA000 && addr <= 0xBFFF)
    {
        if (mRAMEnabled)
            return mRAMBanks[mSelectedRAMBank][addr - 0xA000];
        else
            return 0xFF;
    }

    return mROMMemory[addr];
}

// from addresses 0x800C-0x1000F (the original 0x8000 RAM in all games + 0xB bytes in registers and 0x1 byte in saving the state of the master interrupt),
// the RAM from the memory controller's RAM banks will be stored
// the memory stored contains all of the memory banks, as well as the state of the ram/rom at the time of saving
void MBC::saveRAMToFile(std::ofstream& file)
{
    // the first three bytes starting at the mbc offset contain the states of the ram and rom
    file.write((char*)&mRAMEnabled, 1);
    file.write((char*)&mSelectedRAMBank, 1);
    file.write((char*)&mSelectedROMBank, 1);

    for (int bank = 0; bank < mNumOfRamBanks; bank++)
        file.write((char*)mRAMBanks[bank], RAM_BANK_SIZE);
}

void MBC::setRAMFromFile(std::ifstream& file)
{
    char ramEnabled, ramBank, romBank;
    file.seekg(SAVE_FILE_MBC_OFFSET, file.beg);

    // the first 3 bytes saved store the state of the ram and rom
    file.read(&ramEnabled, 1);
    file.read(&ramBank, 1);
    file.read(&romBank, 1);

    mRAMEnabled = ramEnabled;
    mSelectedRAMBank = ramBank;
    mSelectedROMBank = romBank;

    if (mSelectedROMBank == 0)
        mSelectedROMBank = 1;

    char* dataBuffer = new char[RAM_BANK_SIZE * mNumOfRamBanks];

    file.read(dataBuffer, RAM_BANK_SIZE * mNumOfRamBanks);

    for (int bank = 0; bank < mNumOfRamBanks; bank++)
        for (int byte = RAM_BANK_SIZE * bank; byte < RAM_BANK_SIZE * bank + RAM_BANK_SIZE; byte++)
            mRAMBanks[bank][byte - RAM_BANK_SIZE * bank] = dataBuffer[byte];
}