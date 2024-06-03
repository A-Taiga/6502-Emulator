#ifndef MACROS_HPP
#define MACROS_HPP

#include <cstdint>
#include <array>
#define RAM_SIZE 65536 // 65kb

using byte = std::uint8_t;
using word = std::uint16_t;




[[maybe_unused]] constexpr byte C = 1 << 0;
[[maybe_unused]] constexpr byte Z = 1 << 1;
[[maybe_unused]] constexpr byte I = 1 << 2;
[[maybe_unused]] constexpr byte D = 1 << 3;
[[maybe_unused]] constexpr byte B = 1 << 4;
[[maybe_unused]] constexpr byte V = 1 << 6;
[[maybe_unused]] constexpr byte N = 1 << 7;
[[maybe_unused]] constexpr word ZRO_BEGIN  = 0x0000;
[[maybe_unused]] constexpr word STK_BEGIN  = 0x0100;
[[maybe_unused]] constexpr word ROM_BEGIN  = 0xF000;
[[maybe_unused]] constexpr word ZRO_END    = 0x00FF;
[[maybe_unused]] constexpr word STK_END    = 0x01FF;
[[maybe_unused]] constexpr word ROM_END    = 0xFFFA;



struct Link
{
    bool running;
    std::array<byte, RAM_SIZE>* memory;
};



#endif