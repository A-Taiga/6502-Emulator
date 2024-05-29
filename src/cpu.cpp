#include "cpu.hpp"
#include <stdexcept>
#include <cstdio>
#include <iostream>
#include <format>
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
    if (std::fseek (file, 0L, SEEK_END) == -1)              throw std::runtime_error(strerror(errno));
    if ((size = std::ftell(file)) == -1UL)                  throw std::runtime_error(strerror(errno));
    if (size > memory.size())                               throw std::runtime_error("ROM > 64 kb");
    if ((std::fseek(file, 0L, SEEK_SET)) == -1L)            throw std::runtime_error(strerror(errno));
    if ((std::fread(memory.data(), size, 1, file)) == -1UL) throw std::runtime_error(strerror(errno));
    fclose (file);
}


// void _6502::BRK(void * val)
// {
//     const char* str = (const char*) val;
//     std::cout << str;
// }


/*
addressing	assembler	opc	bytes	cycles
immediate	ADC #oper	69	2	2  
zeropage	ADC oper	65	2	3  
zeropage,X	ADC oper,X	75	2	4  
absolute	ADC oper	6D	3	4  
absolute,X	ADC oper,X	7D	3	4* 
absolute,Y	ADC oper,Y	79	3	4* 
(indirect,X)	ADC (oper,X)	61	2	6  
(indirect),Y	ADC (oper),Y	71	2	5* 

*/

#define PRINT (str) {std::cout << str << std::endl;}




void _6502::decompiler()
{
    opTable[0x00]((void*)"hello world!\n");

    return;

    std::size_t i = 0;

    while (i < memory.size())
    {
        std::cout << std::format("0x{:04X} ", i);

        switch (memory[i])
        {
            case 0x69: std::cout << std::format ("ADC #${:04X}\n", memory[i+1]); i+=2; break; 
            case 0x65: std::cout << std::format ("ADC ${:04X}\n", memory[i+1]); i+=3; break;
            case 0x75: std::cout << std::format ("ADC ${:04X}\n", memory[i+1] + memory[i+2]); i+=3; break;
            case 0x6D: std::cout << std::format ("ADC ${:04X}\n", memory[i+2] | memory[i+1]); i+=4; break;
            case 0x7D:
            
            std::cout << std::format ("ADC ${:04X}\n", (memory[i+2] | memory[i+1]) + memory[i+3]);
            break;
            default: std::cout << std::endl;  i++;  break;
        }
    }
}




