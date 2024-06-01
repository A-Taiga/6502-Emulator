#ifndef EMULATOR_HPP
#define EMULATOR_HPP

#include "macros.hpp"
#include "cpu.hpp"
#include "memory.hpp"

using byte = std::uint8_t;
using word = std::uint16_t;

class Emulator
{
    private:
        ACCESS_MODE rw;
        RAM mem;
        _6502 cpu;
        word addressBus;
        byte dataBus;

    public:
        Emulator (const char* filePath, bool& power);
        const RAM& read_memory() const; // for debug
};

#endif