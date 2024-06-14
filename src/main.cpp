#include "emulator.hpp"


void signal_handler(int signum);
static bool running = true;

int main()
{
    _6502::Emulator emu("testBins/loop5.bin", running);
    emu.run();
    return 0;
}

void signal_handler(int signum)
{
    if (signum == 2)
    {
        running = false;
    }
}