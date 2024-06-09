#include "cpu.hpp"
#include "common.hpp"
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

    29kb = free
*/


_6502::CPU::CPU()
: ins(nullptr)
{
    opcodes =
    {{
        {"BRK", &_6502::CPU::BRK, MODE::IMP, 7}, {"ORA", &_6502::CPU::ORA, MODE::IZX, 6}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"ORA", &_6502::CPU::ORA, MODE::ZPG, 3}, {"ASL", &_6502::CPU::ASL, MODE::ZPG, 5}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"PHP", &_6502::CPU::PHP, MODE::IMP, 3}, {"ORA", &_6502::CPU::ORA, MODE::IMM, 2}, {"ASL", &_6502::CPU::ASL, MODE::IMP, 2}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"ORA", &_6502::CPU::ORA, MODE::ABS, 4}, {"ASL", &_6502::CPU::ASL, MODE::ABS, 6}, {"???", &_6502::CPU::XXX, MODE::IMM, 0},
        {"BPL", &_6502::CPU::BPL, MODE::REL, 2}, {"ORA", &_6502::CPU::ORA, MODE::IZY, 5}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"ORA", &_6502::CPU::ORA, MODE::ZPX, 3}, {"ASL", &_6502::CPU::ASL, MODE::ZPX, 7}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"CLC", &_6502::CPU::CLC, MODE::IMP, 2}, {"ORA", &_6502::CPU::ORA, MODE::ABY, 4}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"ORA", &_6502::CPU::ORA, MODE::ABX, 4}, {"ASL", &_6502::CPU::ASL, MODE::ABX, 7}, {"???", &_6502::CPU::XXX, MODE::IMM, 0},
        {"JSR", &_6502::CPU::JSR, MODE::ABS, 6}, {"AND", &_6502::CPU::AND, MODE::IZX, 6}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"BIT", &_6502::CPU::BIT, MODE::ZPG, 3}, {"AND", &_6502::CPU::AND, MODE::ZPG, 3}, {"ROL", &_6502::CPU::ROL, MODE::ZPG, 5}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"PLP", &_6502::CPU::PLP, MODE::IMP, 4}, {"AND", &_6502::CPU::AND, MODE::IMM, 2}, {"ROL", &_6502::CPU::ROL, MODE::IMP, 2}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"BIT", &_6502::CPU::BIT, MODE::ABS, 4}, {"AND", &_6502::CPU::AND, MODE::ABS, 4}, {"ROL", &_6502::CPU::ROL, MODE::ABS, 6}, {"???", &_6502::CPU::XXX, MODE::IMM, 0},
        {"BMI", &_6502::CPU::BMI, MODE::REL, 2}, {"AND", &_6502::CPU::AND, MODE::IZY, 5}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"AND", &_6502::CPU::AND, MODE::ZPX, 4}, {"ROL", &_6502::CPU::ROL, MODE::ZPX, 6}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"SEC", &_6502::CPU::SEC, MODE::IMP, 2}, {"AND", &_6502::CPU::AND, MODE::ABY, 4}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"AND", &_6502::CPU::AND, MODE::ABX, 4}, {"ROL", &_6502::CPU::ROL, MODE::ABX, 7}, {"???", &_6502::CPU::XXX, MODE::IMM, 0},
        {"RTI", &_6502::CPU::RTI, MODE::IMP, 6}, {"EOR", &_6502::CPU::EOR, MODE::IZX, 6}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"EOR", &_6502::CPU::EOR, MODE::ZPG, 3}, {"LSR", &_6502::CPU::LSR, MODE::ZPG, 5}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"PHA", &_6502::CPU::PHA, MODE::IMP, 3}, {"EOR", &_6502::CPU::EOR, MODE::IMM, 2}, {"LSR", &_6502::CPU::LSR, MODE::IMP, 2}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"JMP", &_6502::CPU::JMP, MODE::ABS, 3}, {"EOR", &_6502::CPU::EOR, MODE::ABS, 4}, {"LSR", &_6502::CPU::LSR, MODE::ABS, 6}, {"???", &_6502::CPU::XXX, MODE::IMM, 0},
        {"BVC", &_6502::CPU::BVC, MODE::REL, 2}, {"EOR", &_6502::CPU::EOR, MODE::IZY, 5}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"EOR", &_6502::CPU::EOR, MODE::ZPX, 4}, {"LSR", &_6502::CPU::LSR, MODE::ZPX, 6}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"CLI", &_6502::CPU::CLI, MODE::IMP, 2}, {"EOR", &_6502::CPU::EOR, MODE::ABY, 4}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"EOR", &_6502::CPU::EOR, MODE::ABX, 4}, {"LSR", &_6502::CPU::LSR, MODE::ABX, 7}, {"???", &_6502::CPU::XXX, MODE::IMM, 0},
        {"RTS", &_6502::CPU::RTS, MODE::IMP, 6}, {"ADC", &_6502::CPU::ADC, MODE::IZX, 6}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"ADC", &_6502::CPU::ADC, MODE::ZPG, 3}, {"ROR", &_6502::CPU::ROR, MODE::ZPG, 5}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"PLA", &_6502::CPU::PLA, MODE::IMP, 4}, {"ADC", &_6502::CPU::ADC, MODE::IMM, 2}, {"ROR", &_6502::CPU::ROR, MODE::IMP, 2}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"JMP", &_6502::CPU::JMP, MODE::IND, 5}, {"ADC", &_6502::CPU::ADC, MODE::ABS, 4}, {"ROR", &_6502::CPU::ROR, MODE::ABS, 6}, {"???", &_6502::CPU::XXX, MODE::IMM, 0},
        {"BVS", &_6502::CPU::BVS, MODE::REL, 2}, {"ADC", &_6502::CPU::ADC, MODE::IZY, 5}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"ADC", &_6502::CPU::ADC, MODE::ZPX, 4}, {"ROR", &_6502::CPU::ROR, MODE::ZPX, 6}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"SEI", &_6502::CPU::SEI, MODE::IMP, 2}, {"ADC", &_6502::CPU::ADC, MODE::ABY, 4}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"ADC", &_6502::CPU::ADC, MODE::ABX, 4}, {"ROR", &_6502::CPU::ROR, MODE::ABX, 7}, {"???", &_6502::CPU::XXX, MODE::IMM, 0},
        {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"STA", &_6502::CPU::STA, MODE::IZX, 6}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"STY", &_6502::CPU::STY, MODE::ZPG, 3}, {"STA", &_6502::CPU::STA, MODE::ZPG, 3}, {"STX", &_6502::CPU::STX, MODE::ZPG, 3}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"DEY", &_6502::CPU::DEY, MODE::IMP, 2}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"TXA", &_6502::CPU::TXA, MODE::IMP, 2}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"STY", &_6502::CPU::STY, MODE::ABS, 4}, {"STA", &_6502::CPU::STA, MODE::ABS, 4}, {"STX", &_6502::CPU::STX, MODE::ABS, 4}, {"???", &_6502::CPU::XXX, MODE::IMM, 0},
        {"BBC", &_6502::CPU::BBC, MODE::REL, 2}, {"STA", &_6502::CPU::STA, MODE::IZY, 5}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"STY", &_6502::CPU::STY, MODE::ZPX, 4}, {"STA", &_6502::CPU::STA, MODE::ZPX, 4}, {"STX", &_6502::CPU::STX, MODE::ZPY, 4}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"TYA", &_6502::CPU::TYA, MODE::IMP, 2}, {"STA", &_6502::CPU::STA, MODE::ABY, 5}, {"TXS", &_6502::CPU::TXS, MODE::IMP, 2}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"STA", &_6502::CPU::STA, MODE::ABX, 5}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0},
        {"LDY", &_6502::CPU::LDY, MODE::IMM, 2}, {"LDA", &_6502::CPU::LDA, MODE::IZX, 6}, {"LDX", &_6502::CPU::LDX, MODE::IMM, 2}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"LDY", &_6502::CPU::LDY, MODE::ZPG, 3}, {"LDA", &_6502::CPU::LDA, MODE::ZPG, 3}, {"LDX", &_6502::CPU::LDX, MODE::ZPG, 3}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"TAY", &_6502::CPU::TAY, MODE::IMP, 2}, {"LDA", &_6502::CPU::LDA, MODE::IMM, 2}, {"TAX", &_6502::CPU::TAX, MODE::IMP, 2}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"LDY", &_6502::CPU::LDY, MODE::ABS, 4}, {"LDA", &_6502::CPU::LDA, MODE::ABS, 4}, {"LDX", &_6502::CPU::LDX, MODE::ABS, 4}, {"???", &_6502::CPU::XXX, MODE::IMM, 0},
        {"BCS", &_6502::CPU::BCS, MODE::REL, 2}, {"LDA", &_6502::CPU::LDA, MODE::IZY, 5}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"LDY", &_6502::CPU::LDY, MODE::ZPX, 4}, {"LDA", &_6502::CPU::LDA, MODE::ZPX, 4}, {"LDX", &_6502::CPU::LDX, MODE::ZPY, 4}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"CLV", &_6502::CPU::CLV, MODE::IMP, 2}, {"LDA", &_6502::CPU::LDA, MODE::ABY, 4}, {"TSX", &_6502::CPU::TSX, MODE::IMP, 2}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"LDY", &_6502::CPU::LDY, MODE::ABX, 4}, {"LDA", &_6502::CPU::LDA, MODE::ABX, 4}, {"LDX", &_6502::CPU::LDX, MODE::ABY, 4}, {"???", &_6502::CPU::XXX, MODE::IMM, 0},
        {"CPY", &_6502::CPU::CPY, MODE::IMM, 2}, {"CMP", &_6502::CPU::CMP, MODE::IZX, 6}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"CPY", &_6502::CPU::CPY, MODE::ZPG, 3}, {"CMP", &_6502::CPU::CMP, MODE::ZPG, 3}, {"DEC", &_6502::CPU::DEC, MODE::ZPG, 5}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"INY", &_6502::CPU::INY, MODE::IMP, 2}, {"CMP", &_6502::CPU::CMP, MODE::IMM, 2}, {"DEX", &_6502::CPU::DEX, MODE::IMP, 2}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"CPY", &_6502::CPU::CPY, MODE::ABS, 4}, {"CMP", &_6502::CPU::CMP, MODE::ABS, 4}, {"DEC", &_6502::CPU::DEC, MODE::ABS, 6}, {"???", &_6502::CPU::XXX, MODE::IMM, 0},
        {"BNE", &_6502::CPU::BNE, MODE::REL, 2}, {"CMP", &_6502::CPU::CMP, MODE::IZY, 5}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"CMP", &_6502::CPU::CMP, MODE::ZPX, 4}, {"DEC", &_6502::CPU::DEC, MODE::ZPX, 6}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"CLD", &_6502::CPU::CLD, MODE::IMP, 2}, {"CMP", &_6502::CPU::CMP, MODE::ABY, 4}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"CMP", &_6502::CPU::CMP, MODE::ABX, 4}, {"DEC", &_6502::CPU::DEC, MODE::ABX, 7}, {"???", &_6502::CPU::XXX, MODE::IMM, 0},
        {"CPX", &_6502::CPU::CPX, MODE::IMM, 2}, {"SBC", &_6502::CPU::SBC, MODE::IZX, 6}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"CPX", &_6502::CPU::CPX, MODE::ZPG, 3}, {"SBC", &_6502::CPU::SBC, MODE::ZPG, 3}, {"INC", &_6502::CPU::INC, MODE::ZPG, 5}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"INX", &_6502::CPU::INX, MODE::IMP, 2}, {"SBC", &_6502::CPU::SPC, MODE::IMM, 2}, {"NOP", &_6502::CPU::NOP, MODE::IMP, 2}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"CPX", &_6502::CPU::CPX, MODE::ABS, 4}, {"SBC", &_6502::CPU::SBC, MODE::ABS, 4}, {"INC", &_6502::CPU::INC, MODE::ABS, 6}, {"???", &_6502::CPU::XXX, MODE::IMM, 0},
        {"BEQ", &_6502::CPU::BEQ, MODE::REL, 2}, {"SBC", &_6502::CPU::SBC, MODE::IZY, 5}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"SBC", &_6502::CPU::SBC, MODE::ZPX, 4}, {"INC", &_6502::CPU::INC, MODE::ZPX, 6}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"SED", &_6502::CPU::SED, MODE::IMP, 2}, {"SBC", &_6502::CPU::SBC, MODE::ABY, 4}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}, {"SBC", &_6502::CPU::SBC, MODE::ABX, 4}, {"INC", &_6502::CPU::INC, MODE::ABX, 7}, {"???", &_6502::CPU::XXX, MODE::IMM, 0}
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
    // for (std::size_t i = ROM_BEGIN; i < ROM_END;)
    // {
    //     // instruction& ins = opcodes[memory[i]];
    //     ins = &opcodes[memory[i]];
    //     std::cout << std::format ("0x{:04X} ", i);
    //     switch (ins->addressMode)
    //     {
    //         case MODE::IMP:
    //         std::cout << std::format ("{:02X} {:>9}\n", memory[i], ins->mnemonic);
    //         i+=1;
    //         break;
    //         case MODE::IMM: 
    //         case MODE::ZPG:
    //         case MODE::ZPX:
    //         case MODE::ZPY:
    //         case MODE::IZX:
    //         case MODE::IZY:
    //         case MODE::REL:
    //         std::cout << std::format ("{:02X} {:02X} {:>6}\n", memory[i], memory[i+1], ins->mnemonic);
    //         i+=2;
    //         break;
    //         case MODE::ABS:
    //         case MODE::ABX:
    //         case MODE::ABY:
    //         case MODE::IND:
    //         std::cout << std::format ("{:02X} {:02X} {:02X} {:}\n", memory[i], memory[i+1], memory[i+1], ins->mnemonic);
    //         i+=3;
    //         break;
    //     }
    // }
}

#include <ncurses.h>
void _6502::CPU::run()
{
    // while (PC < ROM_END)
    // {
    //     // ins = opcodes[link.memory->data()[PC]];
    //     addressMode = ins.addressMode;
    //     (this->*ins.opcode)();
    //     switch (addressMode)
    //     {
    //         case MODE::IMP:
    //         PC+=1;
    //         break;
    //         case MODE::IMM: 
    //         case MODE::ZPG:
    //         case MODE::ZPX:
    //         case MODE::ZPY:
    //         case MODE::IZX:
    //         case MODE::IZY:
    //         case MODE::REL:
    //         PC+=2;
    //         break;
    //         case MODE::ABS:
    //         case MODE::ABX:
    //         case MODE::ABY:
    //         case MODE::IND:
    //         // PC+=3;
    //         break;
    //     }
    //     std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // }
}

const _6502::instruction& _6502::CPU::IF()
{

    return opcodes[0];
}

void _6502::CPU::XXX(void){}
void _6502::CPU::BRK(void){}
void _6502::CPU::ORA(void){}
void _6502::CPU::ASL(void){}
void _6502::CPU::PHP(void){}
void _6502::CPU::BPL(void){}
void _6502::CPU::CLC(void)
{

}
void _6502::CPU::JSR(void){}
void _6502::CPU::AND(void){}
void _6502::CPU::BIT(void){}
void _6502::CPU::ROL(void){}
void _6502::CPU::PLP(void){}
void _6502::CPU::BMI(void){}
void _6502::CPU::SEC(void)
{

}
void _6502::CPU::RTI(void){}
void _6502::CPU::EOR(void){}
void _6502::CPU::LSR(void){}
void _6502::CPU::PHA(void){}
void _6502::CPU::JMP(void)
{
    // if (addressMode == MODE::ABS)
    // {

    //     byte low = link.memory->data()[PC+1];
    //     byte high = link.memory->data()[PC+2]; 
    //     PC = (uint16_t)((high << 8) | low);

    // }
}
void _6502::CPU::BVC(void){}
void _6502::CPU::CLI(void)
{

}
void _6502::CPU::RTS(void){}
void _6502::CPU::PLA(void){}
void _6502::CPU::ADC(void)
{
    // if (addressMode == MODE::IMM)
    // {
    //     AC += link.memory->data()[PC+1];
    // }
}
void _6502::CPU::ROR(void){}
void _6502::CPU::BVS(void){}
void _6502::CPU::SEI(void)
{

}
void _6502::CPU::STA(void)
{
    // if (addressMode == MODE::ZPG)
    // {
    //     byte address = link.memory->data()[PC+1];
    //     link.memory->data()[address] = AC;
    // }
    // else if (addressMode == MODE::ZPX)
    // {
    //     link.memory->data()[X] = AC;
    // }
    // else if (addressMode == MODE::ABS)
    // {
    //     byte low = link.memory->data()[PC+1];
    //     byte high = link.memory->data()[PC+2];
    //     word address = (high << 8) | low;
    //     link.memory->data()[address] = AC;
    //     PC += 3;
    // }
    // else if (addressMode == MODE::IZX)
    // {
    //     byte address = link.memory->data()[PC+1] + X;
    //     link.memory->data()[address] = AC;
    // }
    // else if (addressMode == MODE::ABX)
    // {
    //     byte low = link.memory->data()[PC+1];
    //     byte high = link.memory->data()[PC+2];
    //     word address = ((high << 8) | low) + X;
    //     link.memory->data()[address] = AC; 
    //     PC+=3;
    // }
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
void _6502::CPU::LDA(void){}
void _6502::CPU::LDX(void){}
void _6502::CPU::TAY(void){}
void _6502::CPU::TAX(void){}
void _6502::CPU::BCS(void){}
void _6502::CPU::CLV(void)
{

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