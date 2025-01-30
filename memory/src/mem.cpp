#include "mem.h"
#include <fstream>
#include <iostream>

Memory::RAM::RAM ()
: ram {0}
{
}

Memory::RAM::~RAM(){}

std::uint8_t Memory::RAM::read (const std::uint16_t address) const
{
    return ram[address];
}

void Memory::RAM::write (std::uint16_t address, const std::uint8_t data)
{
    ram[address] = data;
}

void Memory::RAM::reset ()
{
    ram = {0};
}


Memory::RAM::ram_type& Memory::RAM::get_ram ()
{
    return ram;
}


Memory::ROM::ROM()
: rom {0}
, loaded {false}
{}

Memory::ROM::~ROM(){}

bool Memory::ROM::load (const std::string& path, const std::size_t size)
{
    std::fstream file (path);
    if (!file.is_open())
    {
        std::cerr << path << " could not be opened" << std::endl;
        loaded = false;
        return false;
    }

    if (size > Memory::rom_size)
    {
        std::cerr << path << " is larger that max rom size" << std::endl;
        loaded = true;
        return false;
    }

    file.read (reinterpret_cast<char*> (rom.data()), Memory::rom_size);
    file.close();
    return true;
}

std::uint8_t Memory::ROM::read (const std::uint16_t address) const
{
    return rom[address];
}


void Memory::ROM::reset ()
{
    rom = {0};
}


bool Memory::ROM::is_loaded () const
{
    return loaded;
}

Memory::ROM::rom_type& Memory::ROM::get_rom ()
{
    return rom;
}