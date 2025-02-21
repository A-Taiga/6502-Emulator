#include "mem.h"
#include <fstream>
#include <iostream>
#include <print>

Memory::Memory (const std::uint16_t size)
: mem (size, 0)
, loaded {false}
{
}

Memory::~Memory()
{
}

bool Memory::load (const std::string& path, const std::size_t size)
{
    loaded = false;
    std::fstream file (path);
    if (!file.is_open())
    {
        std::cerr << path << " could not be opened" << std::endl;
        loaded = false;
        return false;
    }

    if (size > UINT16_MAX+1)
    {
        std::cerr << path << " is larger that max rom size" << std::endl;
        loaded = false;
        return false;
    }
    mem.resize (size);
    file.read (reinterpret_cast<char*> (mem.data()), mem.size());

    file.close();
    loaded = true;

    return true;
}

std::uint8_t Memory::read (const std::uint16_t address) const
{
    return mem[address];
}

void Memory::write (const std::uint16_t address, const std::uint8_t data)
{
    mem[address] = data;
}

void Memory::reset ()
{
    std::ranges::fill (mem, 0);
}

std::uint8_t* Memory::data ()
{
    return mem.data();
}

Memory::mem_type::iterator Memory::begin ()
{
    return mem.begin();
}

Memory::mem_type::iterator Memory::end ()
{
    return mem.end();
}

std::size_t Memory::size ()
{
    return mem.size();
}

bool Memory::is_loaded () const
{
    return loaded;
}
