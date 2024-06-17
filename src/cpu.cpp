#include "cpu.hpp"
#include "common.hpp"
#include "bus.hpp"
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

[[maybe_unused]] static constexpr word C = 1 << 0;
[[maybe_unused]] static constexpr word Z = 1 << 1;
[[maybe_unused]] static constexpr word I = 1 << 2;
[[maybe_unused]] static constexpr word D = 1 << 3;
[[maybe_unused]] static constexpr word B = 1 << 4;
[[maybe_unused]] static constexpr word V = 1 << 6;
[[maybe_unused]] static constexpr word N = 1 << 7;


_6502::CPU::CPU(Bus& bus)
: bus {bus}
, decompiledCode{}
{
    opcodes = 
    {{
        {"BRK", &_6502::CPU::BRK, &_6502::CPU::IMP, 7}, {"ORA", &_6502::CPU::ORA, &_6502::CPU::IZX, 6}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"ORA", &_6502::CPU::ORA, &_6502::CPU::ZPG, 3}, {"ASL", &_6502::CPU::ASL, &_6502::CPU::ZPG, 5}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"PHP", &_6502::CPU::PHP, &_6502::CPU::IMP, 3}, {"ORA", &_6502::CPU::ORA, &_6502::CPU::IMM, 2}, {"ASL", &_6502::CPU::ASL, &_6502::CPU::IMP, 2}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"ORA", &_6502::CPU::ORA, &_6502::CPU::ABS, 4}, {"ASL", &_6502::CPU::ASL, &_6502::CPU::ABS, 6}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0},
        {"BPL", &_6502::CPU::BPL, &_6502::CPU::REL, 2}, {"ORA", &_6502::CPU::ORA, &_6502::CPU::IZY, 5}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"ORA", &_6502::CPU::ORA, &_6502::CPU::ZPX, 3}, {"ASL", &_6502::CPU::ASL, &_6502::CPU::ZPX, 7}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"CLC", &_6502::CPU::CLC, &_6502::CPU::IMP, 2}, {"ORA", &_6502::CPU::ORA, &_6502::CPU::ABY, 4}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"ORA", &_6502::CPU::ORA, &_6502::CPU::ABX, 4}, {"ASL", &_6502::CPU::ASL, &_6502::CPU::ABX, 7}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0},
        {"JSR", &_6502::CPU::JSR, &_6502::CPU::ABS, 6}, {"AND", &_6502::CPU::AND, &_6502::CPU::IZX, 6}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"BIT", &_6502::CPU::BIT, &_6502::CPU::ZPG, 3}, {"AND", &_6502::CPU::AND, &_6502::CPU::ZPG, 3}, {"ROL", &_6502::CPU::ROL, &_6502::CPU::ZPG, 5}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"PLP", &_6502::CPU::PLP, &_6502::CPU::IMP, 4}, {"AND", &_6502::CPU::AND, &_6502::CPU::IMM, 2}, {"ROL", &_6502::CPU::ROL, &_6502::CPU::IMP, 2}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"BIT", &_6502::CPU::BIT, &_6502::CPU::ABS, 4}, {"AND", &_6502::CPU::AND, &_6502::CPU::ABS, 4}, {"ROL", &_6502::CPU::ROL, &_6502::CPU::ABS, 6}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0},
        {"BMI", &_6502::CPU::BMI, &_6502::CPU::REL, 2}, {"AND", &_6502::CPU::AND, &_6502::CPU::IZY, 5}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"AND", &_6502::CPU::AND, &_6502::CPU::ZPX, 4}, {"ROL", &_6502::CPU::ROL, &_6502::CPU::ZPX, 6}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"SEC", &_6502::CPU::SEC, &_6502::CPU::IMP, 2}, {"AND", &_6502::CPU::AND, &_6502::CPU::ABY, 4}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"AND", &_6502::CPU::AND, &_6502::CPU::ABX, 4}, {"ROL", &_6502::CPU::ROL, &_6502::CPU::ABX, 7}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0},
        {"RTI", &_6502::CPU::RTI, &_6502::CPU::IMP, 6}, {"EOR", &_6502::CPU::EOR, &_6502::CPU::IZX, 6}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"EOR", &_6502::CPU::EOR, &_6502::CPU::ZPG, 3}, {"LSR", &_6502::CPU::LSR, &_6502::CPU::ZPG, 5}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"PHA", &_6502::CPU::PHA, &_6502::CPU::IMP, 3}, {"EOR", &_6502::CPU::EOR, &_6502::CPU::IMM, 2}, {"LSR", &_6502::CPU::LSR, &_6502::CPU::IMP, 2}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"JMP", &_6502::CPU::JMP, &_6502::CPU::ABS, 3}, {"EOR", &_6502::CPU::EOR, &_6502::CPU::ABS, 4}, {"LSR", &_6502::CPU::LSR, &_6502::CPU::ABS, 6}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0},
        {"BVC", &_6502::CPU::BVC, &_6502::CPU::REL, 2}, {"EOR", &_6502::CPU::EOR, &_6502::CPU::IZY, 5}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"EOR", &_6502::CPU::EOR, &_6502::CPU::ZPX, 4}, {"LSR", &_6502::CPU::LSR, &_6502::CPU::ZPX, 6}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"CLI", &_6502::CPU::CLI, &_6502::CPU::IMP, 2}, {"EOR", &_6502::CPU::EOR, &_6502::CPU::ABY, 4}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"EOR", &_6502::CPU::EOR, &_6502::CPU::ABX, 4}, {"LSR", &_6502::CPU::LSR, &_6502::CPU::ABX, 7}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0},
        {"RTS", &_6502::CPU::RTS, &_6502::CPU::IMP, 6}, {"ADC", &_6502::CPU::ADC, &_6502::CPU::IZX, 6}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"ADC", &_6502::CPU::ADC, &_6502::CPU::ZPG, 3}, {"ROR", &_6502::CPU::ROR, &_6502::CPU::ZPG, 5}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"PLA", &_6502::CPU::PLA, &_6502::CPU::IMP, 4}, {"ADC", &_6502::CPU::ADC, &_6502::CPU::IMM, 2}, {"ROR", &_6502::CPU::ROR, &_6502::CPU::IMP, 2}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"JMP", &_6502::CPU::JMP, &_6502::CPU::IND, 5}, {"ADC", &_6502::CPU::ADC, &_6502::CPU::ABS, 4}, {"ROR", &_6502::CPU::ROR, &_6502::CPU::ABS, 6}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0},
        {"BVS", &_6502::CPU::BVS, &_6502::CPU::REL, 2}, {"ADC", &_6502::CPU::ADC, &_6502::CPU::IZY, 5}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"ADC", &_6502::CPU::ADC, &_6502::CPU::ZPX, 4}, {"ROR", &_6502::CPU::ROR, &_6502::CPU::ZPX, 6}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"SEI", &_6502::CPU::SEI, &_6502::CPU::IMP, 2}, {"ADC", &_6502::CPU::ADC, &_6502::CPU::ABY, 4}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"ADC", &_6502::CPU::ADC, &_6502::CPU::ABX, 4}, {"ROR", &_6502::CPU::ROR, &_6502::CPU::ABX, 7}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0},
        {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"STA", &_6502::CPU::STA, &_6502::CPU::IZX, 6}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"STY", &_6502::CPU::STY, &_6502::CPU::ZPG, 3}, {"STA", &_6502::CPU::STA, &_6502::CPU::ZPG, 3}, {"STX", &_6502::CPU::STX, &_6502::CPU::ZPG, 3}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"DEY", &_6502::CPU::DEY, &_6502::CPU::IMP, 2}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"TXA", &_6502::CPU::TXA, &_6502::CPU::IMP, 2}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"STY", &_6502::CPU::STY, &_6502::CPU::ABS, 4}, {"STA", &_6502::CPU::STA, &_6502::CPU::ABS, 4}, {"STX", &_6502::CPU::STX, &_6502::CPU::ABS, 4}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0},
        {"BBC", &_6502::CPU::BBC, &_6502::CPU::REL, 2}, {"STA", &_6502::CPU::STA, &_6502::CPU::IZY, 5}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"STY", &_6502::CPU::STY, &_6502::CPU::ZPX, 4}, {"STA", &_6502::CPU::STA, &_6502::CPU::ZPX, 4}, {"STX", &_6502::CPU::STX, &_6502::CPU::ZPY, 4}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"TYA", &_6502::CPU::TYA, &_6502::CPU::IMP, 2}, {"STA", &_6502::CPU::STA, &_6502::CPU::ABY, 5}, {"TXS", &_6502::CPU::TXS, &_6502::CPU::IMP, 2}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"STA", &_6502::CPU::STA, &_6502::CPU::ABX, 5}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0},
        {"LDY", &_6502::CPU::LDY, &_6502::CPU::IMM, 2}, {"LDA", &_6502::CPU::LDA, &_6502::CPU::IZX, 6}, {"LDX", &_6502::CPU::LDX, &_6502::CPU::IMM, 2}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"LDY", &_6502::CPU::LDY, &_6502::CPU::ZPG, 3}, {"LDA", &_6502::CPU::LDA, &_6502::CPU::ZPG, 3}, {"LDX", &_6502::CPU::LDX, &_6502::CPU::ZPG, 3}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"TAY", &_6502::CPU::TAY, &_6502::CPU::IMP, 2}, {"LDA", &_6502::CPU::LDA, &_6502::CPU::IMM, 2}, {"TAX", &_6502::CPU::TAX, &_6502::CPU::IMP, 2}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"LDY", &_6502::CPU::LDY, &_6502::CPU::ABS, 4}, {"LDA", &_6502::CPU::LDA, &_6502::CPU::ABS, 4}, {"LDX", &_6502::CPU::LDX, &_6502::CPU::ABS, 4}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0},
        {"BCS", &_6502::CPU::BCS, &_6502::CPU::REL, 2}, {"LDA", &_6502::CPU::LDA, &_6502::CPU::IZY, 5}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"LDY", &_6502::CPU::LDY, &_6502::CPU::ZPX, 4}, {"LDA", &_6502::CPU::LDA, &_6502::CPU::ZPX, 4}, {"LDX", &_6502::CPU::LDX, &_6502::CPU::ZPY, 4}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"CLV", &_6502::CPU::CLV, &_6502::CPU::IMP, 2}, {"LDA", &_6502::CPU::LDA, &_6502::CPU::ABY, 4}, {"TSX", &_6502::CPU::TSX, &_6502::CPU::IMP, 2}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"LDY", &_6502::CPU::LDY, &_6502::CPU::ABX, 4}, {"LDA", &_6502::CPU::LDA, &_6502::CPU::ABX, 4}, {"LDX", &_6502::CPU::LDX, &_6502::CPU::ABY, 4}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0},
        {"CPY", &_6502::CPU::CPY, &_6502::CPU::IMM, 2}, {"CMP", &_6502::CPU::CMP, &_6502::CPU::IZX, 6}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"CPY", &_6502::CPU::CPY, &_6502::CPU::ZPG, 3}, {"CMP", &_6502::CPU::CMP, &_6502::CPU::ZPG, 3}, {"DEC", &_6502::CPU::DEC, &_6502::CPU::ZPG, 5}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"INY", &_6502::CPU::INY, &_6502::CPU::IMP, 2}, {"CMP", &_6502::CPU::CMP, &_6502::CPU::IMM, 2}, {"DEX", &_6502::CPU::DEX, &_6502::CPU::IMP, 2}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"CPY", &_6502::CPU::CPY, &_6502::CPU::ABS, 4}, {"CMP", &_6502::CPU::CMP, &_6502::CPU::ABS, 4}, {"DEC", &_6502::CPU::DEC, &_6502::CPU::ABS, 6}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0},
        {"BNE", &_6502::CPU::BNE, &_6502::CPU::REL, 2}, {"CMP", &_6502::CPU::CMP, &_6502::CPU::IZY, 5}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"CMP", &_6502::CPU::CMP, &_6502::CPU::ZPX, 4}, {"DEC", &_6502::CPU::DEC, &_6502::CPU::ZPX, 6}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"CLD", &_6502::CPU::CLD, &_6502::CPU::IMP, 2}, {"CMP", &_6502::CPU::CMP, &_6502::CPU::ABY, 4}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"CMP", &_6502::CPU::CMP, &_6502::CPU::ABX, 4}, {"DEC", &_6502::CPU::DEC, &_6502::CPU::ABX, 7}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0},
        {"CPX", &_6502::CPU::CPX, &_6502::CPU::IMM, 2}, {"SBC", &_6502::CPU::SBC, &_6502::CPU::IZX, 6}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"CPX", &_6502::CPU::CPX, &_6502::CPU::ZPG, 3}, {"SBC", &_6502::CPU::SBC, &_6502::CPU::ZPG, 3}, {"INC", &_6502::CPU::INC, &_6502::CPU::ZPG, 5}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"INX", &_6502::CPU::INX, &_6502::CPU::IMP, 2}, {"SBC", &_6502::CPU::SPC, &_6502::CPU::IMM, 2}, {"NOP", &_6502::CPU::NOP, &_6502::CPU::IMP, 2}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"CPX", &_6502::CPU::CPX, &_6502::CPU::ABS, 4}, {"SBC", &_6502::CPU::SBC, &_6502::CPU::ABS, 4}, {"INC", &_6502::CPU::INC, &_6502::CPU::ABS, 6}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0},
        {"BEQ", &_6502::CPU::BEQ, &_6502::CPU::REL, 2}, {"SBC", &_6502::CPU::SBC, &_6502::CPU::IZY, 5}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"SBC", &_6502::CPU::SBC, &_6502::CPU::ZPX, 4}, {"INC", &_6502::CPU::INC, &_6502::CPU::ZPX, 6}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"SED", &_6502::CPU::SED, &_6502::CPU::IMP, 2}, {"SBC", &_6502::CPU::SBC, &_6502::CPU::ABY, 4}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}, {"SBC", &_6502::CPU::SBC, &_6502::CPU::ABX, 4}, {"INC", &_6502::CPU::INC, &_6502::CPU::ABX, 7}, {"???", &_6502::CPU::XXX, &_6502::CPU::IMM, 0}
    }};
    reset();
}

void _6502::CPU::reset()
{
    PC = ROM_BEGIN; // temp for now
    AC = 0;
    X  = 0;
    Y  = 0;
    SR = 0;
    SP = 0;
}

void _6502::CPU::decompiler()
{
    opcode* current;
    for (word i = 0; i < 4096;)
    {
        word index = ROM_BEGIN + i;
        current = &opcodes[read(i+ROM_BEGIN)];
        if (current->mode == &_6502::CPU::IMP)
        {
            decompiledCode.emplace_back (index,std::format ("{:04X} {:02X} {:>9}", i, read(index), current->mnemonic));
            i++;
        }
        else if (current->mode == &_6502::CPU::IMM
              || current->mode == &_6502::CPU::ZPG
              || current->mode == &_6502::CPU::ZPX
              || current->mode == &_6502::CPU::IZX
              || current->mode == &_6502::CPU::IZY
              || current->mode == &_6502::CPU::REL)
        {
            std::string formatted = std::format ("{:04X} {:02X} {:02X} {:>6}", index, read(index), read(index+1), current->mnemonic);
            if (current->mode == &_6502::CPU::IMM)
                formatted += std::format (" #${:02X}", read(index+1));
            else
                formatted += std::format ("  ${:02X}", read(index+1));

            
            decompiledCode.emplace_back (index, formatted);
            i+=2;
        }
        else if (current->mode == &_6502::CPU::ABS
              || current->mode == &_6502::CPU::ABX
              || current->mode == &_6502::CPU::ABY
              || current->mode == &_6502::CPU::IND)
        {
            std::string formatted = std::format ("{:04X} {:02X} {:02X} {:02X} {:}", index, read(index), read(index+1), read(index+2), current->mnemonic);
            if (current->mode == &_6502::CPU::ABS)
                formatted += std::format ("  ${:02X}, X:${:02X}", read(index+1), read(index+2));
            else if (current->mode == &_6502::CPU::ABX)
                formatted += std::format ("  ${:02X}, X:${:02X}", read(index+1), read(index+2));
            decompiledCode.emplace_back (index, formatted);
            i+=3;
        }
    }
}

void _6502::CPU::run()
{
    current_ins.opcode = &opcodes[read(PC++)];
    current_ins.cycles += current_ins.opcode->cycles;
    current_ins.cycles += (this->*current_ins.opcode->mode)();
    (this->*current_ins.opcode->op)();
}

byte _6502::CPU::read(const word& address)
{
    return bus.cpu_read(address);
}

void _6502::CPU::write (const word& address, const byte& data)
{
    bus.cpu_write(address, data);
}


short _6502::CPU::IMP ()
{
    current_ins.data = AC;
    return 0;
}

short _6502::CPU::IMM ()
{
    current_ins.data = PC++;
    return 0;
}

short _6502::CPU::ABS ()
{
    byte low = read(PC++);
    byte high = read(PC++);
    current_ins.data = (high << 8) | low;
    return 0;
}

short _6502::CPU::ZPG ()
{
    current_ins.data = read(PC++);
    return 0;
}

short _6502::CPU::ABX ()
{
    byte low = read(PC++);
    byte high = read(PC++);
    current_ins.data = ((high << 8 )| low) + X;
    return ((current_ins.data & 0xFF00) != high << 8) ? 1 : 0;
}

short _6502::CPU::ABY ()
{
    byte low = read(PC++);
    byte high = read(PC++);
    current_ins.data = static_cast<word> ((high << 8) | low) + Y;
    return ((current_ins.data & 0xFF00) != high << 8) ? 1 : 0;
}

short _6502::CPU::ZPX ()
{
    byte temp = read (PC++);
    current_ins.data = (temp + X) & 0xFF;
    return 0;
}

short _6502::CPU::ZPY ()
{
    byte temp = read (PC++);
    current_ins.data = (temp + Y) & 0xFF;
    return 0;
}

short _6502::CPU::IND ()
{
    byte low = read (PC++);
    byte high = read (PC++);
    current_ins.data = (high << 8) | low;
    return 0;
}

/*
Mnemonic Examples:
LDA ($70,X)
load the contents of the location given in addresses
"$0070+X" and "$0070+1+X" into A
*/

short _6502::CPU::IZX ()
{
    byte temp = read (PC++);
    byte low  = read ((temp + X) & 0xFF);
    byte high = read ((temp + 1 + X) & 0xFF);
    current_ins.data = (high << 8) | low;
    return 0;
}

short _6502::CPU::IZY ()
{
    byte temp = read (PC++);
    byte low  = read (temp & 0xFF);
    byte high = read ((temp + 1) & 0xFF);
    current_ins.data = ((high << 8) | low) + Y;
    return ((current_ins.data & 0xFF00) != high << 8) ? 1 : 0;
}

short _6502::CPU::REL ()
{
    return 0;
}

void _6502::CPU::XXX(void){}
void _6502::CPU::BRK(void){}
void _6502::CPU::ORA(void){}
void _6502::CPU::ASL(void){}
void _6502::CPU::PHP(void){}
void _6502::CPU::BPL(void){}
void _6502::CPU::CLC(void)
{
    SR &= ~C;
}
void _6502::CPU::JSR(void){}
void _6502::CPU::AND(void)
{

}
void _6502::CPU::BIT(void){}
void _6502::CPU::ROL(void){}
void _6502::CPU::PLP(void){}
void _6502::CPU::BMI(void){}
void _6502::CPU::SEC(void){}
void _6502::CPU::RTI(void){}
void _6502::CPU::EOR(void)
{
    AC = AC ^ read(current_ins.data);
    if (AC == 0)
        SR |= Z;
    if (AC & 0x80)
        SR |= N;
}
void _6502::CPU::LSR(void){}
void _6502::CPU::PHA(void){}
void _6502::CPU::JMP(void)
{
    PC = current_ins.data;
}
void _6502::CPU::BVC(void){}
void _6502::CPU::CLI(void)
{
    SR &= ~I;
}
void _6502::CPU::RTS(void){}
void _6502::CPU::PLA(void){}
void _6502::CPU::ADC(void)
{
    if (current_ins.opcode->mode != &_6502::CPU::IMP)
        AC += read (current_ins.data);

    word temp = AC + read (current_ins.data) + (SR &= C);

    if (temp > 0xFF)
        SR |= C;
    if (temp == 0)
        SR |= Z;
    if (temp & 0x8000)
        SR |= N;

}
void _6502::CPU::ROR(void){}
void _6502::CPU::BVS(void){}
void _6502::CPU::SEI(void){}
void _6502::CPU::STA(void)
{
    write (current_ins.data, AC);
}
void _6502::CPU::STY(void){}
void _6502::CPU::STX(void){}
void _6502::CPU::DEY(void){}
void _6502::CPU::TXA(void){}
void _6502::CPU::STZ(void){}
void _6502::CPU::BBC(void){}
void _6502::CPU::TYA(void){}
void _6502::CPU::TXS(void){}
void _6502::CPU::LDY(void){}
void _6502::CPU::LDA(void)
{
    AC = read (current_ins.data);
}
void _6502::CPU::LDX(void){}
void _6502::CPU::TAY(void){}
void _6502::CPU::TAX(void){}
void _6502::CPU::BCS(void){}
void _6502::CPU::CLV(void)
{
    SR &= ~V;
}
void _6502::CPU::TSX(void){}
void _6502::CPU::CPY(void){}
void _6502::CPU::CMP(void){}
void _6502::CPU::DEC(void){}
void _6502::CPU::INY(void){}
void _6502::CPU::DEX(void){}
void _6502::CPU::BNE(void){}
void _6502::CPU::CLD(void)
{
    SR &= ~D;
}
void _6502::CPU::CPX(void){}
void _6502::CPU::SBC(void){}
void _6502::CPU::INC(void){}
void _6502::CPU::INX(void)
{
    X+=1;
}
void _6502::CPU::SPC(void){}
void _6502::CPU::NOP(void){}
void _6502::CPU::BEQ(void){}
void _6502::CPU::SED(void)
{

}

const word& _6502::CPU::get_pc ()
{
    return PC;
}