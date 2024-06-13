#ifndef MACROS_HPP
#define MACROS_HPP

#include <cstdint>
#include <cstddef>

using byte = std::uint8_t;
using word = std::uint16_t;

static constexpr std::size_t RAM_SIZE = 65536; //65kb

static constexpr word ZRO_BEGIN  = 0x0000;
static constexpr word ZRO_END    = 0x00FF;

static constexpr word STK_BEGIN  = 0x0100;
static constexpr word STK_END    = 0x01FF;

static constexpr word ROM_BEGIN  = 0xF000;
static constexpr word ROM_END    = 0xFFFA;

#endif