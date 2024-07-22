#include "bus.hpp"
#include "cpu.hpp"


MOS_6502::Bus::Bus()
: ram {}
, cpu(*this)
{
}

MOS_6502::Bus::~Bus()
{
}

void MOS_6502::Bus::cpu_write (const word& address, const byte data)
{
    ram[address] = data;
}

byte MOS_6502::Bus::cpu_read (const word& address) const
{
    return ram[address];
}