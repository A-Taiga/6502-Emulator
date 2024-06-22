#include "emulator.hpp"

int main()
{
    _6502::Emulator emu("testBins/branchingTest.bin");
    emu.run();
    return 0;
}