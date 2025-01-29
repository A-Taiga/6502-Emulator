#include "bus.h"
#include <fstream>
#include <iostream>


namespace
{
    void read_rom (const char* file_path, std::array <std::uint8_t, 65534>& memory)
    {
        std::ifstream file (file_path);
        if (!file.is_open())
        {
            std::cerr << "File could not be opened" << std::endl;
            std::exit(EXIT_FAILURE);
        }
        file.read(reinterpret_cast<char*>(memory.data()+0xF000), memory.size());
    }
}

Bus::Bus ()
: memory {}
{
    read_rom ("/home/anthony/Workspace/cpp/6502/roms/jmp_test.bin", memory);
}

Bus::~Bus ()
{

}

void Bus::write (const u16 address , const u8 data)
{
    memory[address] = data;
}

u8 Bus::read  (const u16 address)
{
    return memory[address];
}

void load_rom ()
{

}

std::array <u8, 65534>& Bus::get_memory () {return memory;}














// 6502 Test #04
// Heather Justice 3/12/08
// Tests instructions JMP (both addressing modes) & JSR & RTS.
// Assumes that loads & stores & ORA work with all addressing modes.
// NOTE: Depends on addresses of instructions... Specifically, the "final"
//   address is actually hard-coded at address $0020 (first 4 lines of code).
//   Additionally, a JMP and JSR specify specific addresses.
//
// EXPECTED RESULTS: $40=0x42
//


