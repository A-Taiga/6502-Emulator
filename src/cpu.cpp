#include "cpu.hpp"
#include <cstdint>
#include <stdexcept>
#include <cstdio>
#include <iostream>
#include <format>
#include <ncurses.h>
#include <thread>
#include <chrono>
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

/*
    65 kb total memory
    32 kb ram
    RAM = 0x0000 - 0x7FFF   
        Zero page = 0x0000 - 0x00FF
        Stack     = 0x0100 - 0x01FF

    4 kb ROM
    ROM = F000 - 0xFFFA

    VECTORS = 0xFFFA - 0xFFFF

    29kb = free
*/

namespace
{

    template<std::size_t N>
    std::size_t read_file (const char* path, std::array<std::uint8_t, N>& buffer, word offeset = 0)
    {
        FILE* file = fopen(path, "rb");
        std::size_t size;

        if (file == nullptr)                                        throw std::runtime_error(std::strerror(errno));
        if (std::fseek (file, 0L, SEEK_END) == -1)                  throw std::runtime_error(std::strerror(errno));
        if ((size = std::ftell(file)) == -1UL)                      throw std::runtime_error(std::strerror(errno));
        if (size > N)                                               throw std::runtime_error(std::format("file > {:d}", N));
        if ((std::fseek(file, 0L, SEEK_SET)) == -1L)                throw std::runtime_error(std::strerror(errno));
        if ((std::fread(&buffer[offeset], size, 1, file)) == -1UL)  throw std::runtime_error(std::strerror(errno));
        fclose (file);
        return size;
    }
}

_6502::_6502(const char* filePath, bool& _running)
: running(_running)
{
    opcodes =
    {{
        {"BRK", &_6502::BRK, MODE::IMP, 7}, {"ORA", &_6502::ORA, MODE::IZX, 6}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"ORA", &_6502::ORA, MODE::ZPG, 3}, {"ASL", &_6502::ASL, MODE::ZPG, 5}, {"???", &_6502::XXX, MODE::IMM, 0}, {"PHP", &_6502::PHP, MODE::IMP, 3}, {"ORA", &_6502::ORA, MODE::IMM, 2}, {"ASL", &_6502::ASL, MODE::IMP, 2}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"ORA", &_6502::ORA, MODE::ABS, 4}, {"ASL", &_6502::ASL, MODE::ABS, 6}, {"???", &_6502::XXX, MODE::IMM, 0},
        {"BPL", &_6502::BPL, MODE::REL, 2}, {"ORA", &_6502::ORA, MODE::IZY, 5}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"ORA", &_6502::ORA, MODE::ZPX, 3}, {"ASL", &_6502::ASL, MODE::ZPX, 7}, {"???", &_6502::XXX, MODE::IMM, 0}, {"CLC", &_6502::CLC, MODE::IMP, 2}, {"ORA", &_6502::ORA, MODE::ABY, 4}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"ORA", &_6502::ORA, MODE::ABX, 4}, {"ASL", &_6502::ASL, MODE::ABX, 7}, {"???", &_6502::XXX, MODE::IMM, 0},
        {"JSR", &_6502::JSR, MODE::ABS, 6}, {"AND", &_6502::AND, MODE::IZX, 6}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"BIT", &_6502::BIT, MODE::ZPG, 3}, {"AND", &_6502::AND, MODE::ZPG, 3}, {"ROL", &_6502::ROL, MODE::ZPG, 5}, {"???", &_6502::XXX, MODE::IMM, 0}, {"PLP", &_6502::PLP, MODE::IMP, 4}, {"AND", &_6502::AND, MODE::IMM, 2}, {"ROL", &_6502::ROL, MODE::IMP, 2}, {"???", &_6502::XXX, MODE::IMM, 0}, {"BIT", &_6502::BIT, MODE::ABS, 4}, {"AND", &_6502::AND, MODE::ABS, 4}, {"ROL", &_6502::ROL, MODE::ABS, 6}, {"???", &_6502::XXX, MODE::IMM, 0},
        {"BMI", &_6502::BMI, MODE::REL, 2}, {"AND", &_6502::AND, MODE::IZY, 5}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"AND", &_6502::AND, MODE::ZPX, 4}, {"ROL", &_6502::ROL, MODE::ZPX, 6}, {"???", &_6502::XXX, MODE::IMM, 0}, {"SEC", &_6502::SEC, MODE::IMP, 2}, {"AND", &_6502::AND, MODE::ABY, 4}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"AND", &_6502::AND, MODE::ABX, 4}, {"ROL", &_6502::ROL, MODE::ABX, 7}, {"???", &_6502::XXX, MODE::IMM, 0},
        {"RTI", &_6502::RTI, MODE::IMP, 6}, {"EOR", &_6502::EOR, MODE::IZX, 6}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"EOR", &_6502::EOR, MODE::ZPG, 3}, {"LSR", &_6502::LSR, MODE::ZPG, 5}, {"???", &_6502::XXX, MODE::IMM, 0}, {"PHA", &_6502::PHA, MODE::IMP, 3}, {"EOR", &_6502::EOR, MODE::IMM, 2}, {"LSR", &_6502::LSR, MODE::IMP, 2}, {"???", &_6502::XXX, MODE::IMM, 0}, {"JMP", &_6502::JMP, MODE::ABS, 3}, {"EOR", &_6502::EOR, MODE::ABS, 4}, {"LSR", &_6502::LSR, MODE::ABS, 6}, {"???", &_6502::XXX, MODE::IMM, 0},
        {"BVC", &_6502::BVC, MODE::REL, 2}, {"EOR", &_6502::EOR, MODE::IZY, 5}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"EOR", &_6502::EOR, MODE::ZPX, 4}, {"LSR", &_6502::LSR, MODE::ZPX, 6}, {"???", &_6502::XXX, MODE::IMM, 0}, {"CLI", &_6502::CLI, MODE::IMP, 2}, {"EOR", &_6502::EOR, MODE::ABY, 4}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"EOR", &_6502::EOR, MODE::ABX, 4}, {"LSR", &_6502::LSR, MODE::ABX, 7}, {"???", &_6502::XXX, MODE::IMM, 0},
        {"RTS", &_6502::RTS, MODE::IMP, 6}, {"ADC", &_6502::ADC, MODE::IZX, 6}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"ADC", &_6502::ADC, MODE::ZPG, 3}, {"ROR", &_6502::ROR, MODE::ZPG, 5}, {"???", &_6502::XXX, MODE::IMM, 0}, {"PLA", &_6502::PLA, MODE::IMP, 4}, {"ADC", &_6502::ADC, MODE::IMM, 2}, {"ROR", &_6502::ROR, MODE::IMP, 2}, {"???", &_6502::XXX, MODE::IMM, 0}, {"JMP", &_6502::JMP, MODE::IND, 5}, {"ADC", &_6502::ADC, MODE::ABS, 4}, {"ROR", &_6502::ROR, MODE::ABS, 6}, {"???", &_6502::XXX, MODE::IMM, 0},
        {"BVS", &_6502::BVS, MODE::REL, 2}, {"ADC", &_6502::ADC, MODE::IZY, 5}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"ADC", &_6502::ADC, MODE::ZPX, 4}, {"ROR", &_6502::ROR, MODE::ZPX, 6}, {"???", &_6502::XXX, MODE::IMM, 0}, {"SEI", &_6502::SEI, MODE::IMP, 2}, {"ADC", &_6502::ADC, MODE::ABY, 4}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"ADC", &_6502::ADC, MODE::ABX, 4}, {"ROR", &_6502::ROR, MODE::ABX, 7}, {"???", &_6502::XXX, MODE::IMM, 0},
        {"???", &_6502::XXX, MODE::IMM, 0}, {"STA", &_6502::STA, MODE::IZX, 6}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"STY", &_6502::STY, MODE::ZPG, 3}, {"STA", &_6502::STA, MODE::ZPG, 3}, {"STX", &_6502::STX, MODE::ZPG, 3}, {"???", &_6502::XXX, MODE::IMM, 0}, {"DEY", &_6502::DEY, MODE::IMP, 2}, {"???", &_6502::XXX, MODE::IMM, 0}, {"TXA", &_6502::TXA, MODE::IMP, 2}, {"???", &_6502::XXX, MODE::IMM, 0}, {"STY", &_6502::STY, MODE::ABS, 4}, {"STA", &_6502::STA, MODE::ABS, 4}, {"STX", &_6502::STX, MODE::ABS, 4}, {"???", &_6502::XXX, MODE::IMM, 0},
        {"BBC", &_6502::BBC, MODE::REL, 2}, {"STA", &_6502::STA, MODE::IZY, 5}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"STY", &_6502::STY, MODE::ZPX, 4}, {"STA", &_6502::STA, MODE::ZPX, 4}, {"STX", &_6502::STX, MODE::ZPY, 4}, {"???", &_6502::XXX, MODE::IMM, 0}, {"TYA", &_6502::TYA, MODE::IMP, 2}, {"STA", &_6502::STA, MODE::ABY, 5}, {"TXS", &_6502::TXS, MODE::IMP, 2}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"STA", &_6502::STA, MODE::ABX, 5}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0},
        {"LDY", &_6502::LDY, MODE::IMM, 2}, {"LDA", &_6502::LDA, MODE::IZX, 6}, {"LDX", &_6502::LDX, MODE::IMM, 2}, {"???", &_6502::XXX, MODE::IMM, 0}, {"LDY", &_6502::LDY, MODE::ZPG, 3}, {"LDA", &_6502::LDA, MODE::ZPG, 3}, {"LDX", &_6502::LDX, MODE::ZPG, 3}, {"???", &_6502::XXX, MODE::IMM, 0}, {"TAY", &_6502::TAY, MODE::IMP, 2}, {"LDA", &_6502::LDA, MODE::IMM, 2}, {"TAX", &_6502::TAX, MODE::IMP, 2}, {"???", &_6502::XXX, MODE::IMM, 0}, {"LDY", &_6502::LDY, MODE::ABS, 4}, {"LDA", &_6502::LDA, MODE::ABS, 4}, {"LDX", &_6502::LDX, MODE::ABS, 4}, {"???", &_6502::XXX, MODE::IMM, 0},
        {"BCS", &_6502::BCS, MODE::REL, 2}, {"LDA", &_6502::LDA, MODE::IZY, 5}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"LDY", &_6502::LDY, MODE::ZPX, 4}, {"LDA", &_6502::LDA, MODE::ZPX, 4}, {"LDX", &_6502::LDX, MODE::ZPY, 4}, {"???", &_6502::XXX, MODE::IMM, 0}, {"CLV", &_6502::CLV, MODE::IMP, 2}, {"LDA", &_6502::LDA, MODE::ABY, 4}, {"TSX", &_6502::TSX, MODE::IMP, 2}, {"???", &_6502::XXX, MODE::IMM, 0}, {"LDY", &_6502::LDY, MODE::ABX, 4}, {"LDA", &_6502::LDA, MODE::ABX, 4}, {"LDX", &_6502::LDX, MODE::ABY, 4}, {"???", &_6502::XXX, MODE::IMM, 0},
        {"CPY", &_6502::CPY, MODE::IMM, 2}, {"CMP", &_6502::CMP, MODE::IZX, 6}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"CPY", &_6502::CPY, MODE::ZPG, 3}, {"CMP", &_6502::CMP, MODE::ZPG, 3}, {"DEC", &_6502::DEC, MODE::ZPG, 5}, {"???", &_6502::XXX, MODE::IMM, 0}, {"INY", &_6502::INY, MODE::IMP, 2}, {"CMP", &_6502::CMP, MODE::IMM, 2}, {"DEX", &_6502::DEX, MODE::IMP, 2}, {"???", &_6502::XXX, MODE::IMM, 0}, {"CPY", &_6502::CPY, MODE::ABS, 4}, {"CMP", &_6502::CMP, MODE::ABS, 4}, {"DEC", &_6502::DEC, MODE::ABS, 6}, {"???", &_6502::XXX, MODE::IMM, 0},
        {"BNE", &_6502::BNE, MODE::REL, 2}, {"CMP", &_6502::CMP, MODE::IZY, 5}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"CMP", &_6502::CMP, MODE::ZPX, 4}, {"DEC", &_6502::DEC, MODE::ZPX, 6}, {"???", &_6502::XXX, MODE::IMM, 0}, {"CLD", &_6502::CLD, MODE::IMP, 2}, {"CMP", &_6502::CMP, MODE::ABY, 4}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"CMP", &_6502::CMP, MODE::ABX, 4}, {"DEC", &_6502::DEC, MODE::ABX, 7}, {"???", &_6502::XXX, MODE::IMM, 0},
        {"CPX", &_6502::CPX, MODE::IMM, 2}, {"SBC", &_6502::SBC, MODE::IZX, 6}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"CPX", &_6502::CPX, MODE::ZPG, 3}, {"SBC", &_6502::SBC, MODE::ZPG, 3}, {"INC", &_6502::INC, MODE::ZPG, 5}, {"???", &_6502::XXX, MODE::IMM, 0}, {"INX", &_6502::INX, MODE::IMP, 2}, {"SBC", &_6502::SPC, MODE::IMM, 2}, {"NOP", &_6502::NOP, MODE::IMP, 2}, {"???", &_6502::XXX, MODE::IMM, 0}, {"CPX", &_6502::CPX, MODE::ABS, 4}, {"SBC", &_6502::SBC, MODE::ABS, 4}, {"INC", &_6502::INC, MODE::ABS, 6}, {"???", &_6502::XXX, MODE::IMM, 0},
        {"BEQ", &_6502::BEQ, MODE::REL, 2}, {"SBC", &_6502::SBC, MODE::IZY, 5}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"SBC", &_6502::SBC, MODE::ZPX, 4}, {"INC", &_6502::INC, MODE::ZPX, 6}, {"???", &_6502::XXX, MODE::IMM, 0}, {"SED", &_6502::SED, MODE::IMP, 2}, {"SBC", &_6502::SBC, MODE::ABY, 4}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"???", &_6502::XXX, MODE::IMM, 0}, {"SBC", &_6502::SBC, MODE::ABX, 4}, {"INC", &_6502::INC, MODE::ABX, 7}, {"???", &_6502::XXX, MODE::IMM, 0}
    }};
    reset();
    read_file(filePath, memory.mem, ROM_BEGIN);

}

void _6502::reset()
{
    PC = ROM_BEGIN;
    AC = 0;
    X  = 0;
    Y  = 0;
    SR = 0;
    SP = 0;
    cycles = 0;
    memory.mem = {0};
}

void _6502::decompiler()
{
    for (std::size_t i = ROM_BEGIN; i < ROM_END;)
    {
        instruction& ins = opcodes[memory[i]];
        std::cout << std::format ("0x{:04X} ", i);
        switch (ins.addressMode)
        {
            case MODE::IMP:
            std::cout << std::format ("{:02X} {:>9}\n", memory[i], ins.mnemonic);
            i+=1;
            break;
            case MODE::IMM: 
            case MODE::ZPG:
            case MODE::ZPX:
            case MODE::ZPY:
            case MODE::IZX:
            case MODE::IZY:
            case MODE::REL:
            std::cout << std::format ("{:02X} {:02X} {:>6}\n", memory[i], memory[i+1], ins.mnemonic);
            i+=2;
            break;
            case MODE::ABS:
            case MODE::ABX:
            case MODE::ABY:
            case MODE::IND:
            std::cout << std::format ("{:02X} {:02X} {:02X} {:}\n", memory[i], memory[i+1], memory[i+1], ins.mnemonic);
            i+=3;
            break;
        }
    }
}

void _6502::run()
{
    initscr();
    cbreak();
    while (PC < ROM_END && running)
    {
        instruction& ins = opcodes[memory[PC]];
        addressMode = ins.addressMode;
        (this->*ins.opcode)();
        switch (addressMode)
        {
            case MODE::IMP:
            PC+=1;
            break;
            case MODE::IMM: 
            case MODE::ZPG:
            case MODE::ZPX:
            case MODE::ZPY:
            case MODE::IZX:
            case MODE::IZY:
            case MODE::REL:
            PC+=2;
            break;
            case MODE::ABS:
            case MODE::ABX:
            case MODE::ABY:
            case MODE::IND:
            // PC+=3;
            break;
        }

        // printf("PC = %04X AC = %02X  [0x0001] = %02X\r", PC, AC, memory[1]);
        // std::cout << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
    }
    endwin();
}

void _6502::XXX(void){}
void _6502::BRK(void){}
void _6502::ORA(void){}
void _6502::ASL(void){}
void _6502::PHP(void){}
void _6502::BPL(void){}
void _6502::CLC(void)
{

}
void _6502::JSR(void){}
void _6502::AND(void){}
void _6502::BIT(void){}
void _6502::ROL(void){}
void _6502::PLP(void){}
void _6502::BMI(void){}
void _6502::SEC(void)
{

}
void _6502::RTI(void){}
void _6502::EOR(void){}
void _6502::LSR(void){}
void _6502::PHA(void){}
void _6502::JMP(void)
{
    if (addressMode == MODE::ABS)
        PC = (uint16_t)((memory[PC+2] << 8) | memory[PC+1]);
}
void _6502::BVC(void){}
void _6502::CLI(void)
{

}
void _6502::RTS(void){}
void _6502::PLA(void){}
void _6502::ADC(void)
{
    if (addressMode == MODE::IMM)
    {
        AC += memory[PC+1];
    }
}
void _6502::ROR(void){}
void _6502::BVS(void){}
void _6502::SEI(void)
{

}
void _6502::STA(void)
{
    if (addressMode == MODE::ZPG)
    {
        memory[memory[PC+1]] = AC;
    }
    
}
void _6502::STY(void){}
void _6502::STX(void){}
void _6502::DEY(void){}
void _6502::TXA(void){}
void _6502::STZ(void){}
void _6502::BBC(void){}
void _6502::TYA(void){}
void _6502::TXS(void){}
void _6502::LDY(void){}
void _6502::LDA(void){}
void _6502::LDX(void){}
void _6502::TAY(void){}
void _6502::TAX(void){}
void _6502::BCS(void){}
void _6502::CLV(void)
{

}
void _6502::TSX(void){}
void _6502::CPY(void){}
void _6502::CMP(void){}
void _6502::DEC(void){}
void _6502::INY(void){}
void _6502::DEX(void){}
void _6502::BNE(void){}
void _6502::CLD(void)
{

}
void _6502::CPX(void){}
void _6502::SBC(void){}
void _6502::INC(void){}
void _6502::INX(void){}
void _6502::SPC(void){}
void _6502::NOP(void){}
void _6502::BEQ(void){}
void _6502::SED(void)
{

}