#ifndef EMULATOR_HPP
#define EMULATOR_HPP

#include "common.hpp"
#include "cpu.hpp"
#include "memory.hpp"

using byte = std::uint8_t;
using word = std::uint16_t;

class Emulator
{
    public:
        RAM mem;
        _6502 cpu;
        Link link;
        [[maybe_unused]] bool& running;
    public:
        Emulator (const char* filePath, bool& power);
        const RAM& read_memory() const; // for debug
        Link& get_link();
};

#endif