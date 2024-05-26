#ifndef CPU_HPP
#define CPU_HPP

#include <cstdint>
#include <array>


/*
    https://en.wikipedia.org/wiki/MOS_Technology_6502

    Like its precursor, the 6800, the 6502 has very few registers. The 6502's registers 
    include one 8-bit accumulator register (A), two 8-bit index registers (X and Y), 7 
    processor status flag bits (P; from bit 7 to bit 0 these are the negative (N), overflow (V), 
    reserved, break (B), decimal (D), interrupt disable (I), zero (Z) and carry (C) flag), an 8-bit stack 
    pointer (S), and a 16-bit program counter (PC).[75] This compares to a contemporaneous competitor, the Intel 
    8080, which likewise has one 8-bit accumulator and a 16-bit program counter, but has six more general-purpose 8-bit 
    registers (which can be combined into three 16-bit pointers), and a larger 16-bit stack pointer
*/


/*
    https://www.masswerk.at/6502/6502_instruction_set.html

    PC	program counter	(16 bit)
    AC	accumulator	(8 bit)
    X	X register	(8 bit)
    Y	Y register	(8 bit)
    SR	status register [NV-BDIZC]	(8 bit)
    SP	stack pointer	(8 bit)

    Note: The status register (SR) is also known as the P register.


    SR Flags (bit 7 to bit 0)
    N	Negative
    V	Overflow
    -	ignored
    B	Break
    D	Decimal (use BCD for arithmetics)
    I	Interrupt (IRQ disable)
    Z	Zero
    C	
    Processor Stack
    LIFO, top-down, 8 bit range, 0x0100 - 0x01FF
*/



struct _6502
{
    // main registers
    std::uint16_t PC;   // program counter
    std::uint8_t  AC;   // accumulator
    std::uint8_t  x;    // x register
    std::uint8_t  y;    // y register
    std::uint8_t  SR;   // status register [NV-BDIZC] (aka flags)
    std::uint8_t  SP;   // stack pointer
    std::array<std::uint8_t, 65536> memory; // 256 pages of 256 bits each

    void load_rom(const char* filePath);
    void disassembler();
};


#endif

