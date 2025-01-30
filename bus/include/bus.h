#ifndef BUS_H
#define BUS_H

#include <cstdint>


namespace Memory
{
    class ROM;
    class RAM;
}

class Bus
{

public:
    Bus (Memory::ROM& _rom, Memory::RAM& _ram);
    ~Bus ();

    void write (const std::uint16_t address, const std::uint8_t data);
    std::uint8_t   read  (const std::uint16_t address);

private:
    Memory::ROM& rom;
    Memory::RAM& ram;
};


#endif