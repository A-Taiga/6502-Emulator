#include "emulator.hpp"

int main()
{
    std::setlocale(LC_CTYPE, ".UTF8");
    bool running = true;
    _6502::Emulator emu("testBins/branchingTest.bin", running);
    emu.run();
    return 0;
}