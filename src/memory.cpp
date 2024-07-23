#include "memory.hpp"
#include "common.hpp"

MOS_6502::RAM::RAM()
: ram{0}
{

}

byte& MOS_6502::RAM::operator[](const word index)
{
    return ram[index];
}

const byte& MOS_6502::RAM::operator[](const word index) const
{
    return ram[index];
}

void MOS_6502::RAM::reset ()
{
    ram = {0};
}

MOS_6502::RAM::type& MOS_6502::RAM::get_ram ()
{
    return ram;
}
