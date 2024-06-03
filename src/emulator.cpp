#include "emulator.hpp"

Emulator::Emulator(const char* filePath, bool& power)
: mem(filePath, link)
, cpu(link)
, running(power)
{
    link = Link(power, &mem.data());
    cpu.start();

}

const RAM& Emulator::read_memory() const
{
    return mem;
}

Link& Emulator::get_link()
{
    return link;
}
