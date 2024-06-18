#include "cpu.hpp"
#include "common.hpp"
#include "bus.hpp"
#include <cstdint>
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


namespace
{

    [[maybe_unused]] const word C = 1 << 0;
    [[maybe_unused]] const word Z = 1 << 1;
    [[maybe_unused]] const word I = 1 << 2;
    [[maybe_unused]] const word D = 1 << 3;
    [[maybe_unused]] const word B = 1 << 4;
    [[maybe_unused]] const word V = 1 << 6;
    [[maybe_unused]] const word N = 1 << 7;

    using CPU = _6502::CPU;
    using x = _6502::Address_Type;
    std::array <_6502::opcode,256> opcodes
    ({
        {"BRK", &CPU::BRK, &CPU::IMP, x::IMP, 7}, {"ORA", &CPU::ORA, &CPU::IZX, x::IZX, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"ORA", &CPU::ORA, &CPU::ZPG, x::ZPG, 3}, {"ASL", &CPU::ASL, &CPU::ZPG, x::ZPG, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"PHP", &CPU::PHP, &CPU::IMP, x::IMP, 3}, {"ORA", &CPU::ORA, &CPU::IMM, x::IMM, 2}, {"ASL", &CPU::ASL, &CPU::IMP, x::IMP, 2}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"ORA", &CPU::ORA, &CPU::ABS, x::ABS, 4}, {"ASL", &CPU::ASL, &CPU::ABS, x::ABS, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"BPL", &CPU::BPL, &CPU::REL, x::REL, 2}, {"ORA", &CPU::ORA, &CPU::IZY, x::IZY, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"ORA", &CPU::ORA, &CPU::ZPX, x::ZPX, 3}, {"ASL", &CPU::ASL, &CPU::ZPX, x::ZPX, 7}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"CLC", &CPU::CLC, &CPU::IMP, x::IMP, 2}, {"ORA", &CPU::ORA, &CPU::ABY, x::ABY, 4}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"ORA", &CPU::ORA, &CPU::ABX, x::ABX, 4}, {"ASL", &CPU::ASL, &CPU::ABX, x::ABX, 7}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"JSR", &CPU::JSR, &CPU::ABS, x::ABS, 6}, {"AND", &CPU::AND, &CPU::IZX, x::IZX, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"BIT", &CPU::BIT, &CPU::ZPG, x::ZPG, 3}, {"AND", &CPU::AND, &CPU::ZPG, x::ZPG, 3}, {"ROL", &CPU::ROL, &CPU::ZPG, x::ZPG, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"PLP", &CPU::PLP, &CPU::IMP, x::IMP, 4}, {"AND", &CPU::AND, &CPU::IMM, x::IMM, 2}, {"ROL", &CPU::ROL, &CPU::IMP, x::IMP, 2}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"BIT", &CPU::BIT, &CPU::ABS, x::ABS, 4}, {"AND", &CPU::AND, &CPU::ABS, x::ABS, 4}, {"ROL", &CPU::ROL, &CPU::ABS, x::ABS, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"BMI", &CPU::BMI, &CPU::REL, x::REL, 2}, {"AND", &CPU::AND, &CPU::IZY, x::IZY, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"AND", &CPU::AND, &CPU::ZPX, x::ZPX, 4}, {"ROL", &CPU::ROL, &CPU::ZPX, x::ZPX, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"SEC", &CPU::SEC, &CPU::IMP, x::IMP, 2}, {"AND", &CPU::AND, &CPU::ABY, x::ABY, 4}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"AND", &CPU::AND, &CPU::ABX, x::ABX, 4}, {"ROL", &CPU::ROL, &CPU::ABX, x::ABX, 7}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"RTI", &CPU::RTI, &CPU::IMP, x::IMP, 6}, {"EOR", &CPU::EOR, &CPU::IZX, x::IZX, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"EOR", &CPU::EOR, &CPU::ZPG, x::ZPG, 3}, {"LSR", &CPU::LSR, &CPU::ZPG, x::ZPG, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"PHA", &CPU::PHA, &CPU::IMP, x::IMP, 3}, {"EOR", &CPU::EOR, &CPU::IMM, x::IMM, 2}, {"LSR", &CPU::LSR, &CPU::IMP, x::IMP, 2}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"JMP", &CPU::JMP, &CPU::ABS, x::ABS, 3}, {"EOR", &CPU::EOR, &CPU::ABS, x::ABS, 4}, {"LSR", &CPU::LSR, &CPU::ABS, x::ABS, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"BVC", &CPU::BVC, &CPU::REL, x::REL, 2}, {"EOR", &CPU::EOR, &CPU::IZY, x::IZY, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"EOR", &CPU::EOR, &CPU::ZPX, x::ZPX, 4}, {"LSR", &CPU::LSR, &CPU::ZPX, x::ZPX, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"CLI", &CPU::CLI, &CPU::IMP, x::IMP, 2}, {"EOR", &CPU::EOR, &CPU::ABY, x::ABY, 4}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"EOR", &CPU::EOR, &CPU::ABX, x::ABX, 4}, {"LSR", &CPU::LSR, &CPU::ABX, x::ABX, 7}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"RTS", &CPU::RTS, &CPU::IMP, x::IMP, 6}, {"ADC", &CPU::ADC, &CPU::IZX, x::IZX, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"ADC", &CPU::ADC, &CPU::ZPG, x::ZPG, 3}, {"ROR", &CPU::ROR, &CPU::ZPG, x::ZPG, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"PLA", &CPU::PLA, &CPU::IMP, x::IMP, 4}, {"ADC", &CPU::ADC, &CPU::IMM, x::IMM, 2}, {"ROR", &CPU::ROR, &CPU::IMP, x::IMP, 2}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"JMP", &CPU::JMP, &CPU::IND, x::IND, 5}, {"ADC", &CPU::ADC, &CPU::ABS, x::ABS, 4}, {"ROR", &CPU::ROR, &CPU::ABS, x::ABS, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"BVS", &CPU::BVS, &CPU::REL, x::REL, 2}, {"ADC", &CPU::ADC, &CPU::IZY, x::IZY, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"ADC", &CPU::ADC, &CPU::ZPX, x::ZPX, 4}, {"ROR", &CPU::ROR, &CPU::ZPX, x::ZPX, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"SEI", &CPU::SEI, &CPU::IMP, x::IMP, 2}, {"ADC", &CPU::ADC, &CPU::ABY, x::ABY, 4}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"ADC", &CPU::ADC, &CPU::ABX, x::ABX, 4}, {"ROR", &CPU::ROR, &CPU::ABX, x::ABX, 7}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"STA", &CPU::STA, &CPU::IZX, x::IZX, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"STY", &CPU::STY, &CPU::ZPG, x::ZPG, 3}, {"STA", &CPU::STA, &CPU::ZPG, x::ZPG, 3}, {"STX", &CPU::STX, &CPU::ZPG, x::ZPG, 3}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"DEY", &CPU::DEY, &CPU::IMP, x::IMP, 2}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"TXA", &CPU::TXA, &CPU::IMP, x::IMP, 2}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"STY", &CPU::STY, &CPU::ABS, x::ABS, 4}, {"STA", &CPU::STA, &CPU::ABS, x::ABS, 4}, {"STX", &CPU::STX, &CPU::ABS, x::ABS, 4}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"BBC", &CPU::BBC, &CPU::REL, x::REL, 2}, {"STA", &CPU::STA, &CPU::IZY, x::IZY, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"STY", &CPU::STY, &CPU::ZPX, x::ZPX, 4}, {"STA", &CPU::STA, &CPU::ZPX, x::ZPX, 4}, {"STX", &CPU::STX, &CPU::ZPY, x::ZPY, 4}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"TYA", &CPU::TYA, &CPU::IMP, x::IMP, 2}, {"STA", &CPU::STA, &CPU::ABY, x::ABY, 5}, {"TXS", &CPU::TXS, &CPU::IMP, x::IMP, 2}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"STA", &CPU::STA, &CPU::ABX, x::ABX, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"LDY", &CPU::LDY, &CPU::IMM, x::IMM, 2}, {"LDA", &CPU::LDA, &CPU::IZX, x::IZX, 6}, {"LDX", &CPU::LDX, &CPU::IMM, x::IMM, 2}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"LDY", &CPU::LDY, &CPU::ZPG, x::ZPG, 3}, {"LDA", &CPU::LDA, &CPU::ZPG, x::ZPG, 3}, {"LDX", &CPU::LDX, &CPU::ZPG, x::ZPG, 3}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"TAY", &CPU::TAY, &CPU::IMP, x::IMP, 2}, {"LDA", &CPU::LDA, &CPU::IMM, x::IMM, 2}, {"TAX", &CPU::TAX, &CPU::IMP, x::IMP, 2}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"LDY", &CPU::LDY, &CPU::ABS, x::ABS, 4}, {"LDA", &CPU::LDA, &CPU::ABS, x::ABS, 4}, {"LDX", &CPU::LDX, &CPU::ABS, x::ABS, 4}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"BCS", &CPU::BCS, &CPU::REL, x::REL, 2}, {"LDA", &CPU::LDA, &CPU::IZY, x::IZY, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"LDY", &CPU::LDY, &CPU::ZPX, x::ZPX, 4}, {"LDA", &CPU::LDA, &CPU::ZPX, x::ZPX, 4}, {"LDX", &CPU::LDX, &CPU::ZPY, x::ZPY, 4}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"CLV", &CPU::CLV, &CPU::IMP, x::IMP, 2}, {"LDA", &CPU::LDA, &CPU::ABY, x::ABY, 4}, {"TSX", &CPU::TSX, &CPU::IMP, x::IMP, 2}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"LDY", &CPU::LDY, &CPU::ABX, x::ABX, 4}, {"LDA", &CPU::LDA, &CPU::ABX, x::ABX, 4}, {"LDX", &CPU::LDX, &CPU::ABY, x::ABY, 4}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"CPY", &CPU::CPY, &CPU::IMM, x::IMM, 2}, {"CMP", &CPU::CMP, &CPU::IZX, x::IZX, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"CPY", &CPU::CPY, &CPU::ZPG, x::ZPG, 3}, {"CMP", &CPU::CMP, &CPU::ZPG, x::ZPG, 3}, {"DEC", &CPU::DEC, &CPU::ZPG, x::ZPG, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"INY", &CPU::INY, &CPU::IMP, x::IMP, 2}, {"CMP", &CPU::CMP, &CPU::IMM, x::IMM, 2}, {"DEX", &CPU::DEX, &CPU::IMP, x::IMP, 2}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"CPY", &CPU::CPY, &CPU::ABS, x::ABS, 4}, {"CMP", &CPU::CMP, &CPU::ABS, x::ABS, 4}, {"DEC", &CPU::DEC, &CPU::ABS, x::ABS, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"BNE", &CPU::BNE, &CPU::REL, x::REL, 2}, {"CMP", &CPU::CMP, &CPU::IZY, x::IZY, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"CMP", &CPU::CMP, &CPU::ZPX, x::ZPX, 4}, {"DEC", &CPU::DEC, &CPU::ZPX, x::ZPX, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"CLD", &CPU::CLD, &CPU::IMP, x::IMP, 2}, {"CMP", &CPU::CMP, &CPU::ABY, x::ABY, 4}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"CMP", &CPU::CMP, &CPU::ABX, x::ABX, 4}, {"DEC", &CPU::DEC, &CPU::ABX, x::ABX, 7}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"CPX", &CPU::CPX, &CPU::IMM, x::IMM, 2}, {"SBC", &CPU::SBC, &CPU::IZX, x::IZX, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"CPX", &CPU::CPX, &CPU::ZPG, x::ZPG, 3}, {"SBC", &CPU::SBC, &CPU::ZPG, x::ZPG, 3}, {"INC", &CPU::INC, &CPU::ZPG, x::ZPG, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"INX", &CPU::INX, &CPU::IMP, x::IMP, 2}, {"SBC", &CPU::SPC, &CPU::IMM, x::IMM, 2}, {"NOP", &CPU::NOP, &CPU::IMP, x::IMP, 2}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"CPX", &CPU::CPX, &CPU::ABS, x::ABS, 4}, {"SBC", &CPU::SBC, &CPU::ABS, x::ABS, 4}, {"INC", &CPU::INC, &CPU::ABS, x::ABS, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"BEQ", &CPU::BEQ, &CPU::REL, x::REL, 2}, {"SBC", &CPU::SBC, &CPU::IZY, x::IZY, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"SBC", &CPU::SBC, &CPU::ZPX, x::ZPX, 4}, {"INC", &CPU::INC, &CPU::ZPX, x::ZPX, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"SED", &CPU::SED, &CPU::IMP, x::IMP, 2}, {"SBC", &CPU::SBC, &CPU::ABY, x::ABY, 4}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"SBC", &CPU::SBC, &CPU::ABX, x::ABX, 4}, {"INC", &CPU::INC, &CPU::ABX, x::ABX, 7}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}
    });
}

_6502::CPU::CPU(Bus& bus)
: bus {bus}
, decompiledCode {}
{
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
    word i = 0;
    while (i < 4096)
    {
        word index = ROM_BEGIN+i;
        current = &opcodes[read(i+ROM_BEGIN)];
        switch (current->addrType)
        {
            case _6502::Address_Type::IMP: decompiledCode.emplace_back(index,std::format ("{:04X} {:02X} {:>9}", index, read(index), current->mnemonic));  i+=1; 
            break;
            case _6502::Address_Type::IMM: decompiledCode.emplace_back(index,std::format ("{:04X} {:02X} {:02X} {:>6} #${:02X}", index, read(index), read(index+1), current->mnemonic, read(index+1)));  i+=2; 
            break;
            case _6502::Address_Type::ABS: decompiledCode.emplace_back(index,std::format ("{:04X} {:02X} {:02X} {:02X} {:} ${:04X} ", index, read(index), read(index+1), read(index+2), current->mnemonic, (read(index+2) << 8) | read(index+1)));  i+=3; 
            break;
            case _6502::Address_Type::ZPG: decompiledCode.emplace_back(index,std::format ("{:04X} {:02X} {:02X} {:>6} ${:02X}", index, read(index), read(index+1), current->mnemonic, read(index+1)));  i+=2; 
            break;
            case _6502::Address_Type::ABX: decompiledCode.emplace_back(index,std::format ("{:04X} {:02X} {:02X} {:02X} {:} ${:02X}, X:${:02X}", index, read(index), read(index+1), read(index+2), current->mnemonic, read(index+1), read(index+2)));  i+=3; 
            break;
            case _6502::Address_Type::ABY: decompiledCode.emplace_back(index,std::format ("{:04X} {:02X} {:02X} {:02X} {:}", index, read(index), read(index+1), read(index+2), current->mnemonic));  i+=3; 
            break;
            case _6502::Address_Type::ZPX: decompiledCode.emplace_back(index,std::format ("{:04X} {:02X} {:02X} {:>6} ${:02X}", index, read(index), read(index+1), current->mnemonic, read(index+1)));  i+=2; 
            break;
            case _6502::Address_Type::ZPY: decompiledCode.emplace_back(index,std::format ("{:04X} {:02X} {:02X} {:>6}", index, read(index), read(index+1), current->mnemonic));  i+=2; 
            break;
            case _6502::Address_Type::IND: decompiledCode.emplace_back(index,std::format ("{:04X} {:02X} {:02X} {:02X} {:}", index, read(index), read(index+1), read(index+2), current->mnemonic));  i+=3; 
            break;
            case _6502::Address_Type::IZX: decompiledCode.emplace_back(index,std::format ("{:04X} {:02X} {:02X} {:>6} ${:02X}", index, read(index), read(index+1), current->mnemonic, read(index+1)));  i+=2; 
            break;
            case _6502::Address_Type::IZY: decompiledCode.emplace_back(index,std::format ("{:04X} {:02X} {:02X} {:>6} ${:02X}", index, read(index), read(index+1), current->mnemonic, read(index+1)));  i+=2; 
            break;
            case _6502::Address_Type::REL: decompiledCode.emplace_back(index,std::format ("{:04X} {:02X} {:02X} {:>6}", index, read(index), read(index+1), current->mnemonic)); i+=2; 
            break;
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