#ifndef BUS_H
#define BUS_H

#include <cstdint>


class Memory;

class Bus
{

public:
    Bus (Memory& _rom, Memory& _ram);
    ~Bus ();

    void write (const std::uint16_t address, const std::uint8_t data);
    std::uint8_t   read  (const std::uint16_t address);

private:
    Memory& rom;
    Memory& ram;
};


#endif