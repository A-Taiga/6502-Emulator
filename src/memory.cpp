#include "memory.hpp"
#include "common.hpp"
#include <mutex>


_6502::RAM::RAM()
: ram{0}
, m{}
{
}

byte& _6502::RAM::operator[](const word index)
{
    std::lock_guard<std::mutex> lk(m);
    return ram[index];
}

byte _6502::RAM::operator[](const word index) const
{
    return ram[index];
}

void _6502::RAM::reset ()
{
    ram = {0};
}

_6502::RAM::type& _6502::RAM::get_ram ()
{
    return ram;
}


// _6502::RAM::type& _6502::RAM::data()
// {
//     return ram;
// }

