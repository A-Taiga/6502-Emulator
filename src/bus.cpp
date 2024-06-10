#include "bus.hpp"
#include "cpu.hpp"


_6502::Bus::Bus()
: cpu(*this)
, ram {}
{
}

_6502::Bus::~Bus()
{

}

void _6502::Bus::cpu_write (const word& address, byte data)
{
    ram[address] = data;
}

byte _6502::Bus::cpu_read (const word& address) const
{
    return ram[address];
}