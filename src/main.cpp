#include "emulator.hpp"

int main()
{
    _6502::Emulator emu("testBins/stackTest.bin");
    emu.run();
    return 0;
}