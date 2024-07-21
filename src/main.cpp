#include "emulator.hpp"

int main()
{
    _6502::Emulator emu("testBins/isr_test.bin");
    emu.run();
    return 0;
}