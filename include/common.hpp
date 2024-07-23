#ifndef MACROS_HPP
#define MACROS_HPP

#include <cstdint>
#include <cstddef>

using byte = std::uint8_t;
using word = std::uint16_t;

static const std::size_t RAM_SIZE        = 65536; //65kb
static const std::size_t PAGE_SIZE       = 256;
static const std::size_t KEY_BUFFER_SIZE = 256;

static const word RESET_VECTOR     = 0xFFFC;
static const word IRQ_VECTOR       = 0xFFFE;

static const word ZRO_BEGIN  = 0x0000;
static const word STK_BEGIN  = 0x0100;
static const word ROM_BEGIN  = 0xF000;
static const word KEY_BEGIN  = 0x0200;


#endif