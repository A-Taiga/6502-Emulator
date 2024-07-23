#include "emulator.hpp"

int main()
{
    MOS_6502::Emulator emu("testBins/key_input.bin");
    emu.run();
    return 0;
}