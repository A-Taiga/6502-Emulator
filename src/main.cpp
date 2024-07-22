#include "emulator.hpp"

int main()
{
    MOS_6502::Emulator emu("testBins/isr_test.bin");
    emu.run();
    return 0;
}