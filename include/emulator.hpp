#ifndef EMULATOR_HPP
#define EMULATOR_HPP

#include "bus.hpp"


using byte = std::uint8_t;
using word = std::uint16_t;

namespace UI{class Window_interface;}
namespace MOS_6502
{
    class Emulator
    {
        private:
            std::string currentFile;
        public:
            Bus bus;
            Emulator (const char* filePath);
            void reset ();
            void run();
            static void impl_ui (void* uData);
    };
}

#endif