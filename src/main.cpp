#include <iostream>
#include "cpu.hpp"

int main()
{

    _6502 cpu;
    cpu.load_rom("6502_functional_test.bin");
    cpu.decompiler();


}