#include "emulator.hpp"
#include "macros.hpp"

Emulator::Emulator(const char* filePath, bool& power)
: rw (ACCESS_MODE::NONE)
, mem(filePath, addressBus, dataBus, rw)
, cpu(power, addressBus, dataBus, rw)
{
    cpu.run();
}

const RAM& Emulator::read_memory() const
{
    return mem;
}
