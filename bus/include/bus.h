#ifndef BUS_H
#define BUS_H

#include <cstdint>
#include <array>
#include <filesystem>
#include <iostream>
#include <fstream>

using u16 = std::uint16_t;
using u8  = std::uint8_t;



class Bus
{

public:
    Bus ();
    ~Bus ();
    void write (const u16 address, const u8 data);
    u8   read  (const u16 address);

    void load_rom ();

    std::array <u8, 65534>& get_memory ();

private:
    std::array<u8, 65534> memory;
};


#endif