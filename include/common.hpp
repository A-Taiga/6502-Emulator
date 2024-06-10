#ifndef MACROS_HPP
#define MACROS_HPP

#include <cstdint>

#define RAM_SIZE 65536 // 65kb

using byte = std::uint8_t;
using word = std::uint16_t;



[[maybe_unused]] constexpr word ZRO_BEGIN  = 0x0000;
[[maybe_unused]] constexpr word STK_BEGIN  = 0x0100;
[[maybe_unused]] constexpr word ROM_BEGIN  = 0xF000;
[[maybe_unused]] constexpr word ZRO_END    = 0x00FF;
[[maybe_unused]] constexpr word STK_END    = 0x01FF;
[[maybe_unused]] constexpr word ROM_END    = 0xFFFA;



#endif