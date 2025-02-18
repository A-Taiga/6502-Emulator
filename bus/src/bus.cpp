#include "bus.h"
#include "mem.h"


// static constexpr std::size_t rom_size = UINT16_MAX / 2;
static constexpr std::size_t ram_size = UINT16_MAX / 2;

Bus::Bus (Memory& _rom, Memory& _ram)
: rom {_rom}
, ram {_ram}
{
}

Bus::~Bus ()
{
}

void Bus::write (const std::uint16_t address, const std::uint8_t data)
{
    if (address > ram_size)
        return;
    ram.write (address, data);
}

std::uint8_t Bus::read  (const std::uint16_t address)
{
    if (address > ram_size)
        return rom.read(0x7FFF & address);
    return ram.read(address);
}




// 6502 Test #04
// Heather Justice 3/12/08
// Tests instructions JMP (both addressing modes) & JSR & RTS.
// Assumes that loads & stores & ORA work with all addressing modes.
// NOTE: Depends on addresses of instructions... Specifically, the "final"
//   address is actually hard-coded at address $0020 (first 4 lines of code).
//   Additionally, a JMP and JSR specify specific addresses.
//
// EXPECTED RESULTS: $40=0x42
//


