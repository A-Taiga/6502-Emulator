#include "memory.hpp"
#include "common.hpp"

_6502::RAM::RAM()
: mem{0}
{

}

byte& _6502::RAM::operator[](word index)
{
    return mem[index];
}

const byte& _6502::RAM::operator[](word index) const
{
    return mem[index];
}

void _6502::RAM::reset()
{
    mem = {0};
}

std::array<byte, RAM_SIZE>& _6502::RAM::data()
{
    return mem;
}

