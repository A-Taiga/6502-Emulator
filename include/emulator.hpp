#ifndef EMULATOR_HPP
#define EMULATOR_HPP


#include "cpu.hpp"
#include "memory.hpp"

using byte = std::uint8_t;
using word = std::uint16_t;

namespace _6502
{
    class Emulator
    {
        public:
            RAM mem;
            CPU cpu;
            [[maybe_unused]] bool& running;
        public:
            void run();
            Emulator (const char* filePath, bool& _running);
    };
}

#endif