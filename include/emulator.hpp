#ifndef EMULATOR_HPP
#define EMULATOR_HPP

#include "bus.hpp"

using byte = std::uint8_t;
using word = std::uint16_t;

namespace _6502
{
    class Emulator
    {
        private:
            Bus bus;
            std::string currentFile;
        public:
            void run();
            Emulator (const char* filePath);
            void reset ();
    };
}

#endif