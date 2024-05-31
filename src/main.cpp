#include <iostream>
#include "cpu.hpp"

int main()
{
    _6502 cpu("testFile.bin");
    // cpu.decompiler();
    cpu.run();
    return 0;
}