#include "cpu.hpp"
#include <stdexcept>
#include <cstdio>
#include <iostream>
#include <format>
#include <cstring>
/*

N	Negative
V	Overflow
-	ignored
B	Break
D	Decimal (use BCD for arithmetics)
I	Interrupt (IRQ disable)
Z	Zero
C	Carry

*/





namespace
{
    /* flags */
    [[maybe_unused]] constexpr std::uint8_t C = 1 << 0; // carry
    [[maybe_unused]] constexpr std::uint8_t Z = 1 << 1; // zero
    [[maybe_unused]] constexpr std::uint8_t I = 1 << 2; // interrupt
    [[maybe_unused]] constexpr std::uint8_t D = 1 << 3; // decimal
    [[maybe_unused]] constexpr std::uint8_t B = 1 << 4; // break
    [[maybe_unused]] constexpr std::uint8_t V = 1 << 6; // overflow
    [[maybe_unused]] constexpr std::uint8_t N = 1 << 7; // negative

}


void _6502::load_rom(const char* filePath)
{
    FILE* file = fopen(filePath, "rb");
    std::size_t size;

    if (file == nullptr)                                    throw std::runtime_error(std::strerror(errno));
    if (std::fseek (file, 0L, SEEK_END) == -1)              throw std::runtime_error(std::strerror(errno));
    if ((size = std::ftell(file)) == -1UL)                  throw std::runtime_error(std::strerror(errno));
    if (size > memory.size())                               throw std::runtime_error("ROM > 64 kb");
    if ((std::fseek(file, 0L, SEEK_SET)) == -1L)            throw std::runtime_error(std::strerror(errno));
    if ((std::fread(memory.data(), size, 1, file)) == -1UL) throw std::runtime_error(std::strerror(errno));
    fclose (file);
}

#define PRINT (str) {std::cout << str << std::endl;}

void _6502::disassembler()
{
    std::size_t i = 0;
    while (i < memory.size())
    {
        // std::cout << std::format("0x{:04X} ", i);
        switch (memory[i])
        {
            /* ADC add memory to accumulator with carry */
            case 0x69: std::cout << std::format ("0x{:04X} {:02X} {:02X}    adc #${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; 
            case 0x65: std::cout << std::format ("0x{:04X} {:02X} {:02X}    adc ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x75: std::cout << std::format ("0x{:04X} {:02X} {:02X}    adc ${:04x}, X\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x6D: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} adc ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            case 0x7D: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} adc ${:02x}{:02x}, X\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break; // *
            case 0x79: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} adc ${:02x}{:02x}, Y\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break; // *
            case 0x61: std::cout << std::format ("0x{:04X} {:02X} {:02X}    adc (${:04x}, X)\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x71: std::cout << std::format ("0x{:04X} {:02X} {:02X}    adc (${:04x}), Y\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; // *
            /* AND  and memory with accumulator */
            case 0x29: std::cout << std::format ("0x{:04X} {:02X} {:02X}    and #${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x25: std::cout << std::format ("0x{:04X} {:02X} {:02X}    and ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x35: std::cout << std::format ("0x{:04X} {:02X} {:02X}    and ${:04x}, X\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x2D: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} and ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            case 0x3D: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} and ${:02x}{:02x}, X\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break; // *
            case 0x39: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} and ${:02x}{:02x}, Y\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break; // *
            case 0x21: std::cout << std::format ("0x{:04X} {:02X} {:02X}    and (${:04x}, X)\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x31: std::cout << std::format ("0x{:04X} {:02X} {:02X}    and (${:04x}), Y\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; // *
            /* ASL shift left one bit (memroy or accumulator) */
            case 0x0A: std::cout << std::format ("0x{:04X} {:02X}       asl, A\n", i, memory[i]); i+=1; break;
            case 0x06: std::cout << std::format ("0x{:04X} {:02X} {:02X}    asl ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x16: std::cout << std::format ("0x{:04X} {:02X} {:02X}    asl ${:04x}, X\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x0E: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} asl ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            case 0x1E: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} asl ${:02x}{:02x}, X\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            /* BCC branch on carry clear */
            case 0x90: std::cout << std::format ("0x{:04X} {:02X} {:02X}    bcc ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; // **
            /* BCS branch on carry set */
            case 0xB0: std::cout << std::format ("0x{:04X} {:02X} {:02X}    bcs ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; // **
            /* BEQ branch on result zero */
            case 0xF0: std::cout << std::format ("0x{:04X} {:02X} {:02X}    beq ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; // **
            /* BIT test bits in memory with accumulator */
            case 0x24: std::cout << std::format ("0x{:04X} {:02X} {:02X}    bit ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x2C: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} bit ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            /* BMI branch on result minus */
            case 0x30: std::cout << std::format ("0x{:04X} {:02X} {:02X}    bmi ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; // **
            /* BNE branch result on zero */
            case 0xD0: std::cout << std::format ("0x{:04X} {:02X} {:02X}    bne ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; // **
            /* BPL branch on result plus */
            case 0x10: std::cout << std::format ("0x{:04X} {:02X} {:02X}    bpl ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; // **
            /* BRK force break */
            case 0x00: std::cout << std::format ("0x{:04X} {:02X}       brk\n", i, memory[i]); i+=1; break;
            /* BVC branch on overflow clear */
            case 0x50: std::cout << std::format ("0x{:04X} {:02X} {:02X}    bvc ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; // **
            /* BVS branch on overflow set */
            case 0x70: std::cout << std::format ("0x{:04X} {:02X} {:02X}    bvs ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; // **
            /* CLC clear carry flag */
            case 0x18: std::cout << std::format ("0x{:04X} {:02X}       cls\n", i, memory[i]); i+=1; break;
            /* CLD clear decimal mode */
            case 0xD8: std::cout << std::format ("0x{:04X} {:02X}       cld\n", i, memory[i]); i+=1; break;
            /* CLI clear interrupt disable bit */
            case 0x58: std::cout << std::format ("0x{:04X} {:02X}       cli\n", i, memory[i]); i+=1; break;
            /* CLV clear overflow flag */
            case 0xB8: std::cout << std::format ("0x{:04X} {:02X}       clv\n", i, memory[i]); i+=1; break;
            /* CMP compare memory with accumulator */
            case 0xC9: std::cout << std::format ("0x{:04X} {:02X} {:02X}    cmp #${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; 
            case 0xC5: std::cout << std::format ("0x{:04X} {:02X} {:02X}    cmp ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xD5: std::cout << std::format ("0x{:04X} {:02X} {:02X}    cmp ${:04x}, X\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xCD: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} cmp ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            case 0xDD: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} cmp ${:02x}{:02x}, X\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break; // *
            case 0xD9: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} cmp ${:02x}{:02x}, Y\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break; // *
            case 0xC1: std::cout << std::format ("0x{:04X} {:02X} {:02X}    cmp (${:04x}, X)\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xD1: std::cout << std::format ("0x{:04X} {:02X} {:02X}    cmp (${:04x}), Y\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; // *
            /* CPX compare memory and index x */
            case 0xE0: std::cout << std::format ("0x{:04X} {:02X} {:02X}    cpx #${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xE4: std::cout << std::format ("0x{:04X} {:02X} {:02X}    cpx ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xEC: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} cpx ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            /* CPY compare memory and index y */
            case 0xC0: std::cout << std::format ("0x{:04X} {:02X} {:02X}    cpy #${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xC4: std::cout << std::format ("0x{:04X} {:02X} {:02X}    cpy ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xCC: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} cpy ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            /* DEC decrement memory by one */
            case 0xC6: std::cout << std::format ("0x{:04X} {:02X} {:02X}    dec ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xD6: std::cout << std::format ("0x{:04X} {:02X} {:02X}    dec ${:04x}, X\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xCE: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} dec ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            case 0xDE: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} dec ${:02x}{:02x}, X\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            /* DEX decrement index x by one */
            case 0xCA: std::cout << std::format ("0x{:04X} {:02X}       dex ${:04x}\n", i, memory[i], memory[i]); i+=1; break;
            /* DEY decrement index y by one */
            case 0x88: std::cout << std::format ("0x{:04X} {:02X}       dey ${:04x}\n", i, memory[i], memory[i]); i+=1; break;
            /* EOR exclusive-OR memory with accumulator */
            case 0x49: std::cout << std::format ("0x{:04X} {:02X} {:02X}    eor #${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; 
            case 0x45: std::cout << std::format ("0x{:04X} {:02X} {:02X}    eor ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x55: std::cout << std::format ("0x{:04X} {:02X} {:02X}    eor ${:04x}, X\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x4D: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} eor ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            case 0x5D: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} eor ${:02x}{:02x}, X\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break; // *
            case 0x59: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} eor ${:02x}{:02x}, Y\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break; // *
            case 0x41: std::cout << std::format ("0x{:04X} {:02X} {:02X}    eor (${:04x}, X)\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x51: std::cout << std::format ("0x{:04X} {:02X} {:02X}    eor (${:04x}), Y\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; // *
            /* INC increment memory by one */
            case 0xE6: std::cout << std::format ("0x{:04X} {:02X} {:02X}    inc ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xF6: std::cout << std::format ("0x{:04X} {:02X} {:02X}    inc ${:04x}, X\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xEE: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} inc ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            case 0xFE: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} inc ${:02x}{:02x}, X\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            /* INX increment index x by one */
            case 0xE8: std::cout << std::format ("0x{:04X} {:02X}       inx ${:04x}\n", i, memory[i], memory[i]); i+=1; break;
            /* INY increment index y by one */
            case 0xC8: std::cout << std::format ("0x{:04X} {:02X}       iny ${:04x}\n", i, memory[i], memory[i]); i+=1; break;
            /* JMP jump to new location */
            case 0x4C: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} jmp ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            case 0x6C: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} jmp (${:02x}{:02x})\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            /* JSR jump to new location saving return address */
            case 0x20: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} jsr ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            /* LDA load accumulator with memory */
            case 0xA9: std::cout << std::format ("0x{:04X} {:02X} {:02X}    lda #${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; 
            case 0xA5: std::cout << std::format ("0x{:04X} {:02X} {:02X}    lda ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xB5: std::cout << std::format ("0x{:04X} {:02X} {:02X}    lda ${:04x}, X\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xAD: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} lda ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            case 0xBD: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} lda ${:02x}{:02x}, X\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break; // *
            case 0xB9: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} lda ${:02x}{:02x}, Y\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break; // *
            case 0xA1: std::cout << std::format ("0x{:04X} {:02X} {:02X}    lda (${:04x}, X)\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xB1: std::cout << std::format ("0x{:04X} {:02X} {:02X}    lda (${:04x}), Y\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; // *
            /* LDX load index x with memory */
            case 0xA2: std::cout << std::format ("0x{:04X} {:02X} {:02X}    ldx #${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xA6: std::cout << std::format ("0x{:04X} {:02X} {:02X}    ldx ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xB6: std::cout << std::format ("0x{:04X} {:02X} {:02X}    ldx ${:04x}, Y\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xAE: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} ldx ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            case 0xBE: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} ldx ${:02x}{:02x}, Y\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break; // *
            /* LDY load index y with memory */
            case 0xA0: std::cout << std::format ("0x{:04X} {:02X} {:02X}    ldy #${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xA4: std::cout << std::format ("0x{:04X} {:02X} {:02X}    ldy ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xB4: std::cout << std::format ("0x{:04X} {:02X} {:02X}    ldy ${:04x}, X\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xAC: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} ldy ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            case 0xBC: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} ldy ${:02x}{:02x}, X\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break; // *
            /* LSR shift one bit right (memory or accumulator) */
            case 0x4A: std::cout << std::format ("0x{:04X} {:02X}       lsr, A\n", i, memory[i]); i+=1; break;
            case 0x46: std::cout << std::format ("0x{:04X} {:02X} {:02X}    lsr ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x56: std::cout << std::format ("0x{:04X} {:02X} {:02X}    lsr ${:04x}, X\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x4E: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} lsr ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            case 0x5E: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} lsr ${:02x}{:02x}, X\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            /* NOP */
            case 0xEA: std::cout << std::format ("0x{:04X} {:02X}       nop\n", i, memory[i]); i+=1; break;
            /* ORA OR memory with accumulator*/
            case 0x09: std::cout << std::format ("0x{:04X} {:02X} {:02X}    ora #${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; 
            case 0x05: std::cout << std::format ("0x{:04X} {:02X} {:02X}    ora ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x15: std::cout << std::format ("0x{:04X} {:02X} {:02X}    ora ${:04x}, X\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x0D: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} ora ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            case 0x1D: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} ora ${:02x}{:02x}, X\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break; // *
            case 0x19: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} ora ${:02x}{:02x}, Y\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break; // *
            case 0x01: std::cout << std::format ("0x{:04X} {:02X} {:02X}    ora (${:04x}, X)\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x11: std::cout << std::format ("0x{:04X} {:02X} {:02X}    ora (${:04x}), Y\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; // *
            /* PHA push accumulator on stack */
            case 0x48: std::cout << std::format ("0x{:04X} {:02X}       pha\n", i, memory[i]); i+=1; break;
            /* PHP push processor status on stack */
            case 0x08: std::cout << std::format ("0x{:04X} {:02X}       php\n", i, memory[i]); i+=1; break;
            /* PLA pull processor status from stack */
            case 0x68: std::cout << std::format ("0x{:04X} {:02X}       pla\n", i, memory[i]); i+=1; break;
            /* PLP pull processor status from stack */
            case 0x28: std::cout << std::format ("0x{:04X} {:02X}       plp\n", i, memory[i]); i+=1; break;
            /* ROL rotate one bit left (memory or accumulator)*/
            case 0x2A: std::cout << std::format ("0x{:04X} {:02X}       rol, A\n", i, memory[i]); i+=1; break;
            case 0x26: std::cout << std::format ("0x{:04X} {:02X} {:02X}    rol ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x36: std::cout << std::format ("0x{:04X} {:02X} {:02X}    rol ${:04x}, X\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x2E: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} rol ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            case 0x3E: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} rol ${:02x}{:02x}, X\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            /* ROR rotate one bit right (memory or accumulator)*/
            case 0x6A: std::cout << std::format ("0x{:04X} {:02X}       ror, A\n", i, memory[i]); i+=1; break;
            case 0x66: std::cout << std::format ("0x{:04X} {:02X} {:02X}    ror ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x76: std::cout << std::format ("0x{:04X} {:02X} {:02X}    ror ${:04x}, X\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x6E: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} ror ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            case 0x7E: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} ror ${:02x}{:02x}, X\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            /* RTI return from interrupt */
            case 0x40: std::cout << std::format ("0x{:04X} {:02X}       rti\n", i, memory[i]); i+=1; break;
            /* RTS return from subroutine */
            case 0x60: std::cout << std::format ("0x{:04X} {:02X}       rts\n", i, memory[i]); i+=1; break;
            /* SBC subtract memory from accumulator with borrow */
            case 0xE9: std::cout << std::format ("0x{:04X} {:02X} {:02X}    sbc #${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; 
            case 0xE5: std::cout << std::format ("0x{:04X} {:02X} {:02X}    sbc ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xF5: std::cout << std::format ("0x{:04X} {:02X} {:02X}    sbc ${:04x}, X\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xED: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} sbc ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            case 0xFD: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} sbc ${:02x}{:02x}, X\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break; // *
            case 0xF9: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} sbc ${:02x}{:02x}, Y\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break; // *
            case 0xE1: std::cout << std::format ("0x{:04X} {:02X} {:02X}    sbc (${:04x}, X)\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0xF1: std::cout << std::format ("0x{:04X} {:02X} {:02X}    sbc (${:04x}), Y\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break; // *
            /* SEC set carry flag */
            case 0x38: std::cout << std::format ("0x{:04X} {:02X}       sec\n", i, memory[i]); i+=1; break;
            /* SED set decimal flag */
            case 0xF8: std::cout << std::format ("0x{:04X} {:02X}       sed\n", i, memory[i]); i+=1; break;
            /* SEI set interrupt disable status */
            case 0x78: std::cout << std::format ("0x{:04X} {:02X}       sei\n", i, memory[i]); i+=1; break;
            /* STA store accumulator in memory */
            case 0x85: std::cout << std::format ("0x{:04X} {:02X} {:02X}    sta ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x95: std::cout << std::format ("0x{:04X} {:02X} {:02X}    sta ${:04x}, X\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x8D: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} sta ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            case 0x9D: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} sta ${:02x}{:02x}, X\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            case 0x99: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} sta ${:02x}{:02x}, Y\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=3; break;
            case 0x81: std::cout << std::format ("0x{:04X} {:02X} {:02X}    sta (${:04x}, X)\n", i, memory[i], memory[i+1], memory[i+1]); i+=3; break;
            case 0x91: std::cout << std::format ("0x{:04X} {:02X} {:02X}    sta (${:04x}), Y\n", i, memory[i], memory[i+1], memory[i+1]); i+=3; break;
            /* STX store index x in memory */
            case 0x86: std::cout << std::format ("0x{:04X} {:02X} {:02X}    stx ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x96: std::cout << std::format ("0x{:04X} {:02X} {:02X}    stx ${:04x}, Y\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x8E: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} stx ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=2; break;
            /* STY store index y in memory */
            case 0x84: std::cout << std::format ("0x{:04X} {:02X} {:02X}    stx ${:04x}\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x94: std::cout << std::format ("0x{:04X} {:02X} {:02X}    stx ${:04x}, X\n", i, memory[i], memory[i+1], memory[i+1]); i+=2; break;
            case 0x8C: std::cout << std::format ("0x{:04X} {:02X} {:02X} {:02X} stx ${:02x}{:02x}\n", i, memory[i], memory[i+1], memory[i+2], memory[i+2], memory[i+1]); i+=2; break;
            /* TAX transfer accumulator to index x */
            case 0xAA: std::cout << std::format ("0x{:04X} {:02X}       tax\n", i, memory[i]); i+=1; break;
            /* TAY transfer accumulator to index y */
            case 0xA8: std::cout << std::format ("0x{:04X} {:02X}       tay\n", i, memory[i]); i+=1; break;
            /* TSX transfer stack pointer to index x */
            case 0xBA: std::cout << std::format ("0x{:04X} {:02X}       tsx\n", i, memory[i]); i+=1; break;
            /* TXA transfer index x to accumulator */
            case 0x8A: std::cout << std::format ("0x{:04X} {:02X}       txa\n", i, memory[i]); i+=1; break;
            /* TXS transfer index x to stack register */
            case 0x9A: std::cout << std::format ("0x{:04X} {:02X}       txs\n", i, memory[i]); i+=1; break;
            /* TYA transfer index y to stack accumulator */
            case 0x98: std::cout << std::format ("0x{:04X} {:02X}       tya\n", i, memory[i]); i+=1; break;
            default: i++;  break;
        }
    }
    std::cout << std::endl;
}