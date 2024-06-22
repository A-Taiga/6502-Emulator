#include "emulator.hpp"

int main()
{
    _6502::Emulator emu("testBins/loop6.bin");
    emu.run();
    return 0;
}