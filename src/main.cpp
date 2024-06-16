#include "emulator.hpp"



int main()
{
    bool running = true;
    _6502::Emulator emu("testBins/loop5.bin", running);
    emu.run();
    return 0;
}
