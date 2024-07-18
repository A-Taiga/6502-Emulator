#include "cpu.hpp"
#include "common.hpp"
#include "bus.hpp"
#include <chrono>
#include <cstring>
#include <format>
#include <thread>

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

    $FFFA, $FFFB ... NMI (Non-Maskable Interrupt) vector
    $FFFC, $FFFD ... RES (Reset) vector
    $FFFE, $FFFF ... IRQ (Interrupt Request) vector

    29kb = free
*/

namespace
{

    using CPU = _6502::CPU;
    using x = _6502::Address_Type;
    constinit std::array <_6502::opcode,256> opcodes
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
        {"BCC", &CPU::BCC, &CPU::REL, x::REL, 2}, {"STA", &CPU::STA, &CPU::IZY, x::IZY, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"STY", &CPU::STY, &CPU::ZPX, x::ZPX, 4}, {"STA", &CPU::STA, &CPU::ZPX, x::ZPX, 4}, {"STX", &CPU::STX, &CPU::ZPY, x::ZPY, 4}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"TYA", &CPU::TYA, &CPU::IMP, x::IMP, 2}, {"STA", &CPU::STA, &CPU::ABY, x::ABY, 5}, {"TXS", &CPU::TXS, &CPU::IMP, x::IMP, 2}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"STA", &CPU::STA, &CPU::ABX, x::ABX, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"LDY", &CPU::LDY, &CPU::IMM, x::IMM, 2}, {"LDA", &CPU::LDA, &CPU::IZX, x::IZX, 6}, {"LDX", &CPU::LDX, &CPU::IMM, x::IMM, 2}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"LDY", &CPU::LDY, &CPU::ZPG, x::ZPG, 3}, {"LDA", &CPU::LDA, &CPU::ZPG, x::ZPG, 3}, {"LDX", &CPU::LDX, &CPU::ZPG, x::ZPG, 3}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"TAY", &CPU::TAY, &CPU::IMP, x::IMP, 2}, {"LDA", &CPU::LDA, &CPU::IMM, x::IMM, 2}, {"TAX", &CPU::TAX, &CPU::IMP, x::IMP, 2}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"LDY", &CPU::LDY, &CPU::ABS, x::ABS, 4}, {"LDA", &CPU::LDA, &CPU::ABS, x::ABS, 4}, {"LDX", &CPU::LDX, &CPU::ABS, x::ABS, 4}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"BCS", &CPU::BCS, &CPU::REL, x::REL, 2}, {"LDA", &CPU::LDA, &CPU::IZY, x::IZY, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"LDY", &CPU::LDY, &CPU::ZPX, x::ZPX, 4}, {"LDA", &CPU::LDA, &CPU::ZPX, x::ZPX, 4}, {"LDX", &CPU::LDX, &CPU::ZPY, x::ZPY, 4}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"CLV", &CPU::CLV, &CPU::IMP, x::IMP, 2}, {"LDA", &CPU::LDA, &CPU::ABY, x::ABY, 4}, {"TSX", &CPU::TSX, &CPU::IMP, x::IMP, 2}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"LDY", &CPU::LDY, &CPU::ABX, x::ABX, 4}, {"LDA", &CPU::LDA, &CPU::ABX, x::ABX, 4}, {"LDX", &CPU::LDX, &CPU::ABY, x::ABY, 4}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"CPY", &CPU::CPY, &CPU::IMM, x::IMM, 2}, {"CMP", &CPU::CMP, &CPU::IZX, x::IZX, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"CPY", &CPU::CPY, &CPU::ZPG, x::ZPG, 3}, {"CMP", &CPU::CMP, &CPU::ZPG, x::ZPG, 3}, {"DEC", &CPU::DEC, &CPU::ZPG, x::ZPG, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"INY", &CPU::INY, &CPU::IMP, x::IMP, 2}, {"CMP", &CPU::CMP, &CPU::IMM, x::IMM, 2}, {"DEX", &CPU::DEX, &CPU::IMP, x::IMP, 2}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"CPY", &CPU::CPY, &CPU::ABS, x::ABS, 4}, {"CMP", &CPU::CMP, &CPU::ABS, x::ABS, 4}, {"DEC", &CPU::DEC, &CPU::ABS, x::ABS, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"BNE", &CPU::BNE, &CPU::REL, x::REL, 2}, {"CMP", &CPU::CMP, &CPU::IZY, x::IZY, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"CMP", &CPU::CMP, &CPU::ZPX, x::ZPX, 4}, {"DEC", &CPU::DEC, &CPU::ZPX, x::ZPX, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"CLD", &CPU::CLD, &CPU::IMP, x::IMP, 2}, {"CMP", &CPU::CMP, &CPU::ABY, x::ABY, 4}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"CMP", &CPU::CMP, &CPU::ABX, x::ABX, 4}, {"DEC", &CPU::DEC, &CPU::ABX, x::ABX, 7}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"CPX", &CPU::CPX, &CPU::IMM, x::IMM, 2}, {"SBC", &CPU::SBC, &CPU::IZX, x::IZX, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"CPX", &CPU::CPX, &CPU::ZPG, x::ZPG, 3}, {"SBC", &CPU::SBC, &CPU::ZPG, x::ZPG, 3}, {"INC", &CPU::INC, &CPU::ZPG, x::ZPG, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"INX", &CPU::INX, &CPU::IMP, x::IMP, 2}, {"SBC", &CPU::SBC, &CPU::IMM, x::IMM, 2}, {"NOP", &CPU::NOP, &CPU::IMP, x::IMP, 2}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"CPX", &CPU::CPX, &CPU::ABS, x::ABS, 4}, {"SBC", &CPU::SBC, &CPU::ABS, x::ABS, 4}, {"INC", &CPU::INC, &CPU::ABS, x::ABS, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0},
        {"BEQ", &CPU::BEQ, &CPU::REL, x::REL, 2}, {"SBC", &CPU::SBC, &CPU::IZY, x::IZY, 5}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"SBC", &CPU::SBC, &CPU::ZPX, x::ZPX, 4}, {"INC", &CPU::INC, &CPU::ZPX, x::ZPX, 6}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"SED", &CPU::SED, &CPU::IMP, x::IMP, 2}, {"SBC", &CPU::SBC, &CPU::ABY, x::ABY, 4}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}, {"SBC", &CPU::SBC, &CPU::ABX, x::ABX, 4}, {"INC", &CPU::INC, &CPU::ABX, x::ABX, 7}, {"???", &CPU::XXX, &CPU::IMM, x::IMM, 0}
    });
}

_6502::CPU::CPU(Bus& bus)
: bus {bus}
, decompiledCode {}
, irq (false)
{
    ins.fetched  = nullptr;
    ins.data     = 0x0000;
}

void _6502::CPU::reset()
{
    AC = 0;
    X  = 0;
    Y  = 0;
    SR = static_cast <flag_type> (FLAG::U);
    SP = 0xFF;
    PC = (bus.ram[RESET_VECTOR+1]  << 8) | bus.ram[RESET_VECTOR];
    set_flag(FLAG::I, true);

}
// $FFFE, $FFFF ... IRQ (Interrupt Request) vector
void _6502::CPU::IRQ ()
{
    if (SR & static_cast <flag_type> (FLAG::I))
    {
        stack_push((PC >> 8) & 0x00FF);
        stack_push(PC & 0x00FF);
		set_flag(FLAG::I, true);
		set_flag(FLAG::U, true);
        set_flag(FLAG::B, false);
        stack_push(SR);
        const word low  = static_cast <word> (read(IRQ_VECTOR));
        const word high = static_cast <word> (read(IRQ_VECTOR+1));
        PC = (high << 8) | low;
    }
}

void _6502::CPU::NMI ()
{

}

void _6502::CPU::decompiler()
{
    word i = 0;
    while (i < 4096)
    {
        word index = ROM_BEGIN+i;
        const opcode& current = opcodes[read(i+ROM_BEGIN)];
        switch (current.addrType)
        {
            case _6502::Address_Type::IMP: decompiledCode.emplace_back(index,std::format ("{:04X}: {:02X} {:>9}", index, read(index), current.mnemonic));  i+=1; break;
            case _6502::Address_Type::IMM: decompiledCode.emplace_back(index,std::format ("{:04X}: {:02X} {:02X} {:>6} #${:02X}", index, read(index), read(index+1), current.mnemonic, read(index+1)));  i+=2; break;
            case _6502::Address_Type::ABS: decompiledCode.emplace_back(index,std::format ("{:04X}: {:02X} {:02X} {:02X} {:} ${:04X} ", index, read(index), read(index+1), read(index+2), current.mnemonic, (read(index+2) << 8) | read(index+1)));  i+=3; break;
            case _6502::Address_Type::ZPG: decompiledCode.emplace_back(index,std::format ("{:04X}: {:02X} {:02X} {:>6} ${:02X}", index, read(index), read(index+1), current.mnemonic, read(index+1)));  i+=2; break;
            case _6502::Address_Type::ABX: decompiledCode.emplace_back(index,std::format ("{:04X}: {:02X} {:02X} {:02X} {:} ${:04X}, X", index, read(index), read(index+1), read(index+2), current.mnemonic, (read(index+2) << 8) | read(index+1)));  i+=3; break;
            case _6502::Address_Type::ABY: decompiledCode.emplace_back(index,std::format ("{:04X}: {:02X} {:02X} {:02X} {:}", index, read(index), read(index+1), read(index+2), current.mnemonic));  i+=3; break;
            case _6502::Address_Type::ZPX: decompiledCode.emplace_back(index,std::format ("{:04X}: {:02X} {:>6} ${:02X}", index, read(index+1), current.mnemonic, read(index+1)));  i+=2; break;
            case _6502::Address_Type::ZPY: decompiledCode.emplace_back(index,std::format ("{:04X}: {:02X} {:02X} {:>6}", index, read(index), read(index+1), current.mnemonic));  i+=2; break;
            case _6502::Address_Type::IND: decompiledCode.emplace_back(index,std::format ("{:04X}: {:02X} {:02X} {:02X} {:}", index, read(index), read(index+1), read(index+2), current.mnemonic));  i+=3; break;
            case _6502::Address_Type::IZX: decompiledCode.emplace_back(index,std::format ("{:04X}: {:02X} {:02X} {:>6} ${:02X}", index, read(index), read(index+1), current.mnemonic, read(index+1)));  i+=2; break;
            case _6502::Address_Type::IZY: decompiledCode.emplace_back(index,std::format ("{:04X}: {:02X} {:02X} {:>6} ${:02X}", index, read(index), read(index+1), current.mnemonic, read(index+1)));  i+=2; break;
            case _6502::Address_Type::REL:
                if (std::strcmp(current.mnemonic, "CPX"))
                {
                    word result = read (index+1);
                    if (result & 0x80) result |= 0xFF00;
                    word addr = index+2 + result;
                    decompiledCode.emplace_back(index,std::format ("{:04X}: {:02X} {:02X} {:>6} ${:04X}", index, read(index), read(index+1), current.mnemonic, addr)); i+=2;
                }
            break;
        }
    }
}

void _6502::CPU::run()
{
    set_flag(FLAG::U, true);
    auto begin = std::chrono::high_resolution_clock::now();
    ins.fetched = &opcodes[read(PC++)];
    ins.cycles = ins.fetched->cycles + (this->*ins.fetched->mode)();
    (this->*ins.fetched->op)();
    set_flag(FLAG::U, true);


    auto end = std::chrono::high_resolution_clock::now();
	if (end - begin > std::chrono::nanoseconds(ins.cycles))
        return;

    std::this_thread::sleep_for(std::chrono::nanoseconds(ins.cycles) - (end - begin));

}

byte _6502::CPU::read(const word address)
{
    return bus.cpu_read(address);
}

void _6502::CPU::write (const word address, const byte data)
{
    bus.cpu_write(address, data);
}

byte _6502::CPU::stack_pop ()
{
    SP++;
    return read (STK_BEGIN + SP); 
}

void _6502::CPU::stack_push (const byte data)
{
    write (STK_BEGIN+SP, data); 
    SP--;
}

void _6502::CPU::set_flag(const FLAG flag, const bool condition)
{
    if (condition)
        SR |= static_cast <flag_type> (flag);
    else
        SR &= ~static_cast<flag_type>(flag);
}

void _6502::CPU::set_pin (const CPU_PINS p)
{
    pins |= static_cast<pin_type> (p);
}

void _6502::CPU::trigger_irq ()
{
    irq = true;
}

int _6502::CPU::IMP ()
{
    ins.data = AC;
    return 0;
}

int _6502::CPU::IMM ()
{
    ins.data = PC++;
    return 0;
}

int _6502::CPU::ABS ()
{
    const byte low = read(PC++);
    const byte high = read(PC++);
    ins.data = (high << 8) | low;
    return 0;
}

int _6502::CPU::ZPG ()
{
    ins.data = read(PC++);
    return 0;
}

int _6502::CPU::ABX ()
{
    const byte low = read(PC++);
    const byte high = read(PC++);
    ins.data = ((high << 8 )| low) + X;
    return ((ins.data & 0xFF00) != high << 8) ? 1 : 0;
}

int _6502::CPU::ABY ()
{
    const byte low = read(PC++);
    const byte high = read(PC++);
    ins.data = static_cast <word> (((high << 8) | low) + Y);
    return ((ins.data & 0xFF00) != high << 8) ? 1 : 0;
}

int _6502::CPU::ZPX ()
{
    const byte temp = read (PC++);
    ins.data = (temp + X) & 0xFF;
    return 0;
}

int _6502::CPU::ZPY ()
{
    const byte temp = read (PC++);
    ins.data = (temp + Y) & 0xFF;
    return 0;
}

int _6502::CPU::IND ()
{
    const byte low = read (PC++);
    const byte high = read (PC++);
    ins.data = (high << 8) | low;
    return 0;
}

/*
Mnemonic Examples:
LDA ($70,X)
load the contents of the location given in addresses
"$0070+X" and "$0070+1+X" into A
*/

int _6502::CPU::IZX ()
{
    const byte temp = read (PC++);
    const byte low  = read ((temp + X) & 0xFF);
    const byte high = read ((temp + 1 + X) & 0xFF);
    ins.data = (high << 8) | low;
    return ((ins.data & 0xFF00) != high << 8) ? 1 : 0;
}

int _6502::CPU::IZY ()
{
    const byte temp = read (PC++);
    const byte low  = read (temp & 0xFF);
    const byte high = read ((temp + 1) & 0xFF);
    ins.data = ((high << 8) | low) + Y;
    return ((ins.data & 0xFF00) != high << 8) ? 1 : 0;
}

int _6502::CPU::REL ()
{
    ins.data = read (PC++);
    if (ins.data & 0x80)
        ins.data |= 0xFF00;
    return 0;
}
/* BRK force break */
void _6502::CPU::BRK(void)
{
    PC++;
    stack_push(PC & 0xFF00);
    stack_push(PC & 0x00FF);
    set_flag(FLAG::B, true);
    stack_push(SR);
    set_flag(FLAG::B, false);
    PC = static_cast <word> (read(0xFFFE)) | static_cast <word> (read(0xFFFF) << 8);
}
/* ORA OR memory with accumulator */
void _6502::CPU::ORA(void)
{
    const word data = static_cast <word> (read (ins.data));
    const word result = AC | data;
    set_flag(FLAG::N, result & 0x0080);
    set_flag(FLAG::Z, (result & 0x00FF) == 0);
    AC = result;
}
/* ASL shift left one bit (memory or accumulator) */
void _6502::CPU::ASL(void)
{
    const word result = ([&]()
    {
        if (ins.fetched->addrType == Address_Type::IMP)
            return static_cast <word> (AC);
        else
            return static_cast <word> (read (ins.data));
    }()) << 1;
    
    // const word result = data << 1;
    set_flag(FLAG::N, result & 0x0080);
    set_flag(FLAG::Z, (result & 0x00FF) == 0);
    set_flag(FLAG::C, result > 0x00FF);

    if (ins.fetched->addrType == Address_Type::IMP)
        AC = result;
    else
        write (ins.data, result & 0x00FF);
}
/* PHP push processor status on stack */
/* http://www.atarihq.com/danb/files/64doc.txt */
void _6502::CPU::PHP(void)
{
    stack_push(SR | static_cast <flag_type> (FLAG::U) | static_cast<flag_type> (FLAG::B));
    set_flag(FLAG::U, false);
    set_flag(FLAG::B, false);
    SP--;
}
/* BPL branch on result plus */
void _6502::CPU::BPL(void)
{
    if (!(SR & static_cast <flag_type> (FLAG::N)))
    {
        ins.cycles++;
        const word addr = PC + ins.data;
        if ((addr & 0xFF00) != (PC & 0xFF00))
            ins.cycles++;
        PC = addr;
    }
}
/* CLC clear carry flag */
void _6502::CPU::CLC(void)
{
    SR &= ~static_cast <flag_type> (FLAG::C);
}
/* JSR jump to new location saving return address */
void _6502::CPU::JSR(void)
{
    PC++;
    stack_push(PC & 0xFF00);
    stack_push(PC & 0x00FF);
    PC = ins.data;
}
/* AND memory with accumulator */
void _6502::CPU::AND(void)
{
    AC &= read(ins.data);
    set_flag(FLAG::N, AC & 0x80);
    set_flag(FLAG::Z, AC == 0x00);
}
/* BIT test bits in memory with accumulator */
void _6502::CPU::BIT(void)
{
    word fetched = static_cast <word> (read(ins.data));
    word result  = static_cast <word> (AC) & fetched;
    set_flag(FLAG::N, fetched & (1 << 7));
    set_flag(FLAG::Z, (result & 0x00FF) == 0x0000);
    set_flag(FLAG::V, fetched & (1 << 6));
}
/* ROL rotate one bit left (memory or accumulator) */
void _6502::CPU::ROL(void)
{
    const word result = static_cast<word >(([&]()
    {
        if (ins.fetched->addrType == Address_Type::IMP)
            return AC;
        else
            return read (ins.data);
    }()) << 1) | (static_cast <flag_type> (FLAG::C) & SP);

    set_flag(FLAG::N, result & 0x0080);
    set_flag(FLAG::Z, (result & 0x00FF) == 0x00);
    set_flag(FLAG::C, result & 0xFF00);

    if (ins.fetched->addrType == Address_Type::IMP)
        AC = result & 0x00FF;
    else
        write (ins.data, result & 0x00FF);
}
/* PLP pull processor status from stack */
void _6502::CPU::PLP(void)
{
    SR = stack_pop();
}
/* BMI branch on result minus */
void _6502::CPU::BMI(void)
{
    if (SR & static_cast <flag_type> (FLAG::N))
    {
        ins.cycles++;
        const word addr = PC + ins.data;
        if ((addr & 0xFF00) != (PC & 0xFF00))
            ins.cycles++;
        PC = addr;
    }
}
/* SEC set carry flag */
void _6502::CPU::SEC(void)
{
    SR |= static_cast <flag_type> (FLAG::C);
}
/* RTI return from interrupt */
void _6502::CPU::RTI(void)
{
    /*
    The status register is pulled with the break flag
    and bit 5 ignored. Then PC is pulled from the stack.
    */
    SR = stack_pop();
    SR &= ~static_cast <flag_type> (FLAG::B);
    SR &= ~static_cast <flag_type> (FLAG::U);
    const word low  = static_cast <word> (stack_pop());
    const word high = static_cast <word> (stack_pop()) << 8;
    PC = high | low;
}
/* XOR memory with accumulator */
void _6502::CPU::EOR(void)
{
    AC |= static_cast <byte> (ins.data);
    set_flag(FLAG::N, AC & 0x80);
    set_flag(FLAG::Z, AC == 0);
}
/* LSR shift one bit right (memory or accumulator) */
void _6502::CPU::LSR(void)
{
    set_flag(FLAG::C, ins.data & 0x0001);
    const word result = ([&]()
    {
        if (ins.fetched->addrType == Address_Type::IMP)
            return static_cast <word> (AC);
        else
            return static_cast <word> (read (ins.data));
    })() >> 1;
    set_flag(FLAG::N, result & 0x80);
    set_flag(FLAG::Z, result == 0x00);
    if (ins.fetched->addrType == Address_Type::IMP)
        AC = result;
    else
        write (ins.data, result & 0x00FF);
}
/* PHA push accumulator on stack */
void _6502::CPU::PHA(void)
{
    stack_push(AC);
}
/* JMP jump to new location */
void _6502::CPU::JMP(void)
{
    PC = ins.data;
}
/* BVC branch on overflow clear */
void _6502::CPU::BVC(void)
{
    if (!(SR & static_cast <flag_type> (FLAG::V)))
    {
        ins.cycles++;
        const word addr = PC + ins.data;
        if ((addr & 0xFF00) != (PC & 0xFF00))
            ins.cycles++;
        PC = addr;
    }
}
/* CLI clear interrupt disable bit */
void _6502::CPU::CLI(void)
{
    set_flag(FLAG::I, false);
}
/* return from subroutine */
void _6502::CPU::RTS(void)
{
    const word low  = static_cast <word> (stack_pop());
    const word high = static_cast <word> (stack_pop()) << 8;
    PC = high | low;
    PC++;
}
/* PLA pull accumulator from stack */
void _6502::CPU::PLA(void)
{
    AC = stack_pop();
    set_flag(FLAG::N, AC & 0x80);
    set_flag(FLAG::Z, AC == 0x00);
}
/* ADC add memory to accumulator with carry */
void _6502::CPU::ADC(void)
{   
    // with regards to the V flag
    // https://stackoverflow.com/questions/29193303/6502-emulation-proper-way-to-implement-adc-and-sbc
    const word fetched =  static_cast <word> (read(ins.data));
    const word result = static_cast <word> (AC) + fetched + static_cast <word> (SR & static_cast <flag_type> (FLAG::C));
    set_flag (FLAG::N, result & 0x0080);
    set_flag (FLAG::Z, (result & 0x00FF) == 0);
    set_flag (FLAG::C, result > 0x00FF);
    set_flag (FLAG::V, ~(AC ^ fetched) & (AC ^ result) & 0x0080);
    AC = result & 0x00FF;
}
/* ROR rotate one bit right (memory or accumulator) */
void _6502::CPU::ROR(void)
{
    const word result = static_cast<word >(([&]()
    {
        if (ins.fetched->addrType == Address_Type::IMP)
            return AC;
        else
            return read (ins.data);
    }()) >> 1) | ((SR & static_cast <flag_type> (FLAG::C)) << 7);
    set_flag(FLAG::N, result & 0x0080);
    set_flag(FLAG::Z, (result & 0x0080) == 0);
    set_flag(FLAG::C, ins.data & 0x01);
    if (ins.fetched->addrType == Address_Type::IMP)
        AC = result & 0x00FF;
    else
        write (ins.data, result & 0x00FF);
}
/* BVS branch on overflow set */
void _6502::CPU::BVS(void)
{
    if (SR & static_cast <flag_type> (FLAG::V))
    {
        ins.cycles++;
        const word addr = PC + ins.data;
        if ((addr & 0xF000) != (PC & 0xFF00))
            ins.cycles++;
        PC = addr;
    }
}
/* SEI set interrupt disable status */
void _6502::CPU::SEI(void)
{
    set_flag (FLAG::I, true);
}
/* store accumulator in memory */
void _6502::CPU::STA(void)
{
    write (ins.data, AC);
}
/* STY store index y in memory */
void _6502::CPU::STY(void)
{
    write (ins.data, Y);
}
/* STX store index x in memory */
void _6502::CPU::STX(void)
{
    write (ins.data, X);
}
/* DEY decrment index y by 1 */
void _6502::CPU::DEY(void)
{
    Y -= 1;
    set_flag(FLAG::N, Y & 0x80);
    set_flag(FLAG::Z, Y == 0x00);
}
/* TXA transfer index x to accumulator */
void _6502::CPU::TXA(void)
{
    AC = X;
    set_flag(FLAG::N, AC & 0x80);
    set_flag(FLAG::Z, AC == 0x00);
}
/* BCC branch on carry clear */
void _6502::CPU::BCC(void)
{
    if (!(SR & static_cast <flag_type> (FLAG::C)))
    {
        ins.cycles++;
        const word addr = PC + ins.data;
        if ((addr & 0xFF00) != (PC & 0xFF00))
            ins.cycles++;
        PC = addr;
    }
}
/* TYA transfer index y to accumulator */
void _6502::CPU::TYA(void)
{
    AC = Y;
    set_flag(FLAG::N, AC & 0x80);
    set_flag(FLAG::Z, AC == 0x00);

}
/* TXS transfer index x to stack register */
void _6502::CPU::TXS(void)
{
    SP = X;
}
/* LDY load index y with memory */
void _6502::CPU::LDY(void)
{
    const word result = static_cast <word> (read(ins.data));
    set_flag(FLAG::N, result & 0x0080);
    set_flag(FLAG::Z, result == 0x0000);
    Y = result & 0x00FF;
}

/* LDA load accumulator with memory */
void _6502::CPU::LDA(void)
{
    const word result = static_cast <word> (read(ins.data));
    set_flag(FLAG::N, result & 0x0080);
    set_flag(FLAG::Z, result == 0x0000);
    AC = result & 0x00FF;
}
/* LDX load index x with memory */
void _6502::CPU::LDX(void) 
{
    const word result = static_cast <word> (read(ins.data));
    set_flag(FLAG::N, result & 0x80);
    set_flag(FLAG::Z, result == 0x00);
    X = result & 0x00FF;
}
/* TAY transfer accumulator to index y */
void _6502::CPU::TAY(void)
{
    Y = AC;
    set_flag(FLAG::N, Y & 0x80);
    set_flag(FLAG::Z, Y == 0x00);
}
/* TAX transfer accumulator to index x */
void _6502::CPU::TAX(void)
{
    X = AC;
    set_flag(FLAG::N, X & 0x80);
    set_flag(FLAG::Z, X == 0x00);
}
/* BCS branch on carry set */
void _6502::CPU::BCS(void)
{
    if (SR & static_cast <flag_type> (FLAG::C))
    {
        ins.cycles++;
        const word addr = PC + ins.data;
        if ((addr & 0xFF00) != (PC & 0xFF00))
            ins.cycles++;
        PC = addr;
    }
}
/* CLV clear overflow flag */
void _6502::CPU::CLV(void)
{
    SR &= ~static_cast <flag_type> (FLAG::V);
}
/* TSX transfer stack pointer to index x */
void _6502::CPU::TSX(void)
{
    X = SP;
    set_flag(FLAG::N, X & 0x80);
    set_flag(FLAG::Z, X == 0x00);
}
/* CPY compare memory with index y */
void _6502::CPU::CPY(void)
{
    const word fetched = read(ins.data);
    const word result = Y - fetched;
    set_flag(FLAG::N, result & 0x0080);
    set_flag(FLAG::Z, result & 0x0000);
    set_flag(FLAG::C, Y >= fetched);
}
/* CMP compare memory with accumulator */
void _6502::CPU::CMP(void)
{
    const word fetched = read(ins.data);
    const word result = AC - fetched;
    set_flag(FLAG::N, result & 0x0080);
    set_flag(FLAG::Z, result & 0x0000);
    set_flag(FLAG::C, Y >= fetched);
}
/* DEC decrment memory by one */
void _6502::CPU::DEC(void)
{
    const word fetched = static_cast <word> (read(ins.data));
    const word result = fetched - 1;
    write (ins.data, result);
    set_flag(FLAG::N, result & 0x0080);
    set_flag(FLAG::Z, (result & 0x00FF) == 0x0000);
}
/* INY increment index y by 1*/
void _6502::CPU::INY(void)
{
    Y += 1;
    set_flag(FLAG::N, Y & 0x80);
    set_flag(FLAG::Z, Y == 0x00);
}
/* DEX decrment index x by 1*/
void _6502::CPU::DEX(void)
{
    X -= 1;
    set_flag(FLAG::N, X & 0x80);
    set_flag(FLAG::Z, X == 0x00);
}
/* BNE branch on result not zero */
void _6502::CPU::BNE(void) 
{
    if (!(SR & static_cast <flag_type> (FLAG::Z)))
    {
        ins.cycles++;
        const word addr = PC + ins.data;
        if ((addr & 0xFF00) != (PC & 0xFF00))
            ins.cycles++;
        PC = addr;
    }
}
/* CLD clear decimal mode */
void _6502::CPU::CLD(void)
{
    SR &= ~static_cast <flag_type> (FLAG::D);
}
/* CPX compare memory with index x*/
void _6502::CPU::CPX(void)
{
    const word fetched = static_cast <word> (read(ins.data));
    const word result = static_cast <word> (X) - fetched;
    set_flag(FLAG::C, X >= fetched);
    set_flag(FLAG::N, result & 0x0080);
    set_flag(FLAG::Z, (result & 0x00FF) == 0x0000);

}
/* SBC subtract memory from accumulator with borrow */
void _6502::CPU::SBC(void)
{
    // https://stackoverflow.com/questions/29193303/6502-emulation-proper-way-to-implement-adc-and-sbc
    const word fetched = static_cast <word> (read(ins.data)) ^ 0x00FF;
    const word result = static_cast <word> (AC) + fetched + (SR & static_cast <flag_type> (FLAG::C));
    set_flag(FLAG::N, result & 0x0080);
    set_flag(FLAG::Z, (result & 0x00FF) == 0);
    set_flag(FLAG::C, result > 0x00FF);
    set_flag(FLAG::V, (AC ^ fetched) & (AC ^ result) & 0x0080);
    AC = result & 0x00FF;
}
/* INC increment memory by one */
void _6502::CPU::INC(void)
{
    const word fetched = static_cast <word> (read(ins.data));
    const word result = fetched + 1;
    write (ins.data, result);
    set_flag(FLAG::N, result & 0x0080);
    set_flag(FLAG::Z, (result & 0x00FF) == 0x0000);
}
/* INX increment index x by 1 */
void _6502::CPU::INX(void)
{
    X += 1;
    set_flag(FLAG::N, X & 0x80);
    set_flag(FLAG::Z, X == 0x00);
}
/* NOP no operation */
void _6502::CPU::NOP(void) {}
/* BEQ branch on equal */
void _6502::CPU::BEQ(void)
{
    if (SR & static_cast <flag_type> (FLAG::Z))
    {
        ins.cycles++;
        const word addr = PC + ins.data;
        if ((addr & 0xFF00) != (PC & 0xFF00))
            ins.cycles++;
        PC = addr;
    }
}
/* SED set decimal flag */
void _6502::CPU::SED(void)
{
    SR |= static_cast <flag_type> (FLAG::D);
}

void _6502::CPU::XXX(void) {}

