#ifndef EMULATOR_HPP
#define EMULATOR_HPP

#include "bus.hpp"

using byte = std::uint8_t;
using word = std::uint16_t;

namespace UI{class Window_Interface;}
namespace _6502
{
    class Emulator
    {
        private:
            std::string currentFile;
        public:
            static Bus bus;
            static bool running;
            static bool pause;
            static bool step;
            void run();
            Emulator (const char* filePath);
            void reset ();
            static void impl_ui (const UI::Window_Interface& window);
    };
}

#endif