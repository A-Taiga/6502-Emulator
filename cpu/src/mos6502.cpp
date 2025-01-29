#include "mos6502.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <format>

MOS_6502::CPU::CPU (read_cb read, write_cb write)
: read{read}
, write{write}
{
    reset ();
}

void MOS_6502::CPU::update (void)
{
    set_flag(Flag::_, true);
    current.pc = PC;
    current.instruction = &instruction_table[read (PC++)];
    current.cycles = current.instruction->cycle_count + (this->*current.instruction->mode)();
    (this->*current.instruction->opcode)();
}

void MOS_6502::CPU::reset (void)
{
    set_flag (Flag::I, true);
    PC = (read(reset_vector) << 8) | read(reset_vector+1);
    AC = 0;
    XR = 0;
    YR = 0;
    SR = 0;
    SP = 0;
    current = {};
}

void MOS_6502::CPU::set_flag(const Flag Flag, const bool condition)
{
    if (condition)
        SR |= static_cast <byte> (Flag);
    else
        SR &= ~static_cast <byte> (Flag);
}

void MOS_6502::CPU::stack_push (const byte data)
{
    // --SP;
    write (stk_begin + SP, data);
    ++SP;
}

byte MOS_6502::CPU::stack_pop (void)
{
    --SP;
    const auto result = read (stk_begin + SP);
    // ++SP;
    return result;
}

/* 
    ADDRESSING MODES 

    some of these functions will return an extra cycle 
    if a page boundry was crossed
*/

// accumulator
int MOS_6502::CPU::ACC (void)
{
    current.data = AC;
    return 0;
}

// absolute
int MOS_6502::CPU::ABS (void)
{
    const byte low = read (PC++);
    const byte high = read (PC++);
    current.address = (high << 8) | low;
    return 0;
}

// absolute XR
int MOS_6502::CPU::ABX (void)
{
    const byte low = read (PC++);
    const byte high = read (PC++);
    current.address = ((high << 8) | low) + XR;
    return (current.address & 0xFF00) != (high << 8) ? 1 : 0;
}

// absolute YR
int MOS_6502::CPU::ABY (void)
{
    const byte low = read (PC++);
    const byte high = read (PC++);
    current.address = ((high << 8) | low) + YR;
    return (current.address & 0xFF00) != (high << 8) ? 1 : 0;
}

// # / immediate 
int MOS_6502::CPU::IMM (void)
{
    current.address = PC++;
    return 0;
}

// implied
int MOS_6502::CPU::IMP (void)
{
    // does nothing?
    return 0;
}

// indirect
int MOS_6502::CPU::IND (void)
{
    const byte low = read (PC++);
    const byte high = read (PC++);
    const word lookup_addr = (high << 8) | low;
    current.address = (read (lookup_addr+1) << 8) | read (lookup_addr);
    return 0;
}

// XR-indexed indirect zeropage address
// operand is zeropage address; effective address is word in (LL + XR, LL + XR + 1), inc. without carry: C.w($00LL + XR)
int MOS_6502::CPU::XIZ (void)
{
    const byte temp = read (PC++);
    const byte low = read (temp + XR);
    const byte high = read (temp + XR + 1);
    current.address = (high << 8) | low;
    return 0;
}


// YR-indexed indirect zeropage address
// operand is zeropage address; effective address is word in (LL, LL + 1) incremented by YR with carry: C.w($00LL) + YR
int MOS_6502::CPU::YIZ (void)
{
    const byte temp = read (PC++);
    const byte low = read (temp);
    const byte high = read (temp + 1);
    current.address = ((high << 8) | low) + YR;
    return (current.address & 0xFF00) != (high << 8) ? 1 : 0;
}

// relative
// branch target is PC + signed offset BB 
int MOS_6502::CPU::REL (void)
{
    current.address = read (PC++);
    current.address |= current.address & 0x80 ? 0xFF00 : 0x0000;
    return 0;
}

// zeropage
int MOS_6502::CPU::ZPG (void)
{
    current.address = read (PC++);
    return 0;
}

// zeropage XR-indexed
int MOS_6502::CPU::ZPX (void)
{
    current.address = read (PC++) + XR;
    return 0;
}

// zeropage YR-indexed
int MOS_6502::CPU::ZPY (void)
{
    current.address = read (PC++) + YR;
    return 0;
}

/* OPCODES */

// break
void MOS_6502::CPU::BRK (void)
{
    ++PC;

    stack_push (PC & 0xFF00);
    stack_push (PC & 0x00FF);

    set_flag (Flag::B, true);
    stack_push (SR);
    set_flag (Flag::B, false);

    set_flag (Flag::I, true);

    PC = read (0xFFFE) | (read (0xFFFF) << 8);
}

// bitwise OR
void MOS_6502::CPU::ORA (void)
{
    AC |= read (current.address);
    set_flag (Flag::Z, AC == 0x00);
    set_flag (Flag::N, AC & 0x80);
}

// arithmetic shift left
void MOS_6502::CPU::ASL (void)
{
    current.data = current.instruction->mode == &CPU::ACC ? AC : read (current.address);
    set_flag (Flag::C, current.data * 0x80);
    current.data <<= 1;
    set_flag (Flag::Z, current.data == 0x00);
    set_flag (Flag::N, current.data & 0x80);
    if (current.instruction->mode == &CPU::ACC)
        AC = current.data;
    else
        write (current.address, current.data);
}

// push processor status
void MOS_6502::CPU::PHP (void)
{
    set_flag (Flag::B, true);
    set_flag (Flag::_, true);
    stack_push (SR);
    set_flag (Flag::B, false);
    set_flag (Flag::_, false);
}

// branch if plus
void MOS_6502::CPU::BPL (void)
{
    if (!(static_cast <byte> (Flag::N) & SR))
    {
        // branch taken so add a cycle
        ++current.cycles;

        current.address += PC;

        // page boundry crossed
        if ((current.address & 0xFF00) != (PC & 0xFF00 ))
            ++current.cycles;
            
        PC = current.address;
    }
}

// clear carry
void MOS_6502::CPU::CLC (void)
{
    SR &= ~static_cast <byte> (Flag::C);
}

// jump to subroutine
void MOS_6502::CPU::JSR (void)
{
    stack_push ((PC >> 8) & 0x00FF);
    stack_push (PC & 0x00FF);
    PC = current.address;
}

// bitwise AND
void MOS_6502::CPU::AND (void)
{
    AC &= read (current.address);
    set_flag (Flag::Z, AC == 0x00);
    set_flag (Flag::N, AC & 0x80);
}

// bit test
void MOS_6502::CPU::BIT (void)
{
    const byte temp = AC & read (current.address);
    
    set_flag (Flag::Z, temp == 0x00);
    set_flag (Flag::V, temp & 0x40);
    set_flag (Flag::N, temp & 0x80);
}

// rotate left
void MOS_6502::CPU::ROL (void)
{
    current.data = current.instruction->mode == &CPU::ACC ? AC : read (current.address);
    
    set_flag (Flag::C, current.data & 0x80);
    
    current.data <<= 1;
    current.data |= static_cast <byte> (Flag::C) & SP;
    
    set_flag (Flag::Z, current.data == 0x00);
    set_flag (Flag::N, current.data & 0x80);
    
    if (current.instruction->mode == &CPU::ACC)
        AC = current.data;
    else
        write (current.address, current.data);
}

// pull processor status
void MOS_6502::CPU::PLP (void)
{
    SR = stack_pop();
}

// branch if minus
void MOS_6502::CPU::BMI (void)
{
    if (!(static_cast <byte> (Flag::N) & SR))
    {
        // branch taken so add cycle
        ++current.cycles;

        current.address += PC;

        // page boundry crossed
        if ((current.address & 0xFF00) != (PC & 0xFF00))
            ++current.cycles;

        PC = current.address;
    }
}

// set carry
void MOS_6502::CPU::SEC (void)
{
    set_flag(Flag::C, true);
}

// return from interrupt
void MOS_6502::CPU::RTI (void)
{
    SR = stack_pop();

    // these two flags are ignored when returning from the stack
    SR &= ~static_cast <byte> (Flag::B);
    SR &= ~static_cast <byte> (Flag::_);

    PC = stack_pop();
    PC |= stack_pop() << 8;
}

// bitwise exclusive OR
void MOS_6502::CPU::EOR (void)
{
    AC ^= read (current.address);
    set_flag (Flag::Z, AC == 0x0);
    set_flag (Flag::N, AC & 0x80);
}

// logical shift right
void MOS_6502::CPU::LSR (void)
{
    current.data = current.instruction->mode == &CPU::ACC ? AC : read (current.address);
    set_flag (Flag::C, current.data & 0x01);
    current.data >>= 1;
    set_flag (Flag::Z, current.data == 0x00);
    set_flag (Flag::N, current.data & 0x80);
    if (current.instruction->mode == &CPU::ACC)
        AC = current.data;
    else
        write (current.address, current.data);
}

// push accumulator
void MOS_6502::CPU::PHA (void)
{
    stack_push (AC);
}

// jump
void MOS_6502::CPU::JMP (void)
{
    PC = current.address;
}

// branch if overflow clear
void MOS_6502::CPU::BVC (void)
{
    if (!(static_cast <byte> (Flag::V) & SR))
    {
        // branching requires an additional cycle
        ++current.cycles;

        current.address += PC;

        // page boundry check
        if ((current.address & 0x00FF) != (PC & 0xFF00))
            ++current.cycles;

        PC = current.address;
    }
}

// clear interrupt disable
void MOS_6502::CPU::CLI (void)
{
    set_flag (Flag::I, false);
}

// return from subroutinef
void MOS_6502::CPU::RTS (void)
{
    const byte low = stack_pop();
    const byte high = stack_pop();
    PC = (high << 8) | low;
}

// pull accumulator
void MOS_6502::CPU::PLA (void)
{
    AC = stack_pop();
    set_flag (Flag::Z, AC == 0x00);
    set_flag (Flag::N, AC & 0x80);
}

// add with carry
void MOS_6502::CPU::ADC (void)
{
    current.data = read (current.address);

    const word result = AC + current.data + (static_cast <byte> (Flag::C) & SR);
    
    set_flag (Flag::C, (result & 0xFF00) != 0);
    set_flag (Flag::Z, result == 0);
    set_flag (Flag::V, ~(result ^ AC) & (result ^ current.data) & 0x0080);
    set_flag (Flag::N, result & 0x0080);
    
    AC = result & 0x00FF;
}

// rotate right
void MOS_6502::CPU::ROR (void)
{
    current.data = current.instruction->mode == &CPU::ACC ? AC : read (current.address);

    set_flag (Flag::C, current.data & 0x80);
    
    current.data >>= 1;
    current.data |= (static_cast <byte> (Flag::C) & SP) << 7;
    
    set_flag (Flag::Z, current.data == 0x00);
    set_flag (Flag::N, current.data & 0x80);

    if (current.instruction->mode == &CPU::ACC)
        AC = current.data;
    else
        write (current.address, current.data);
}

// branch if overflow set
void MOS_6502::CPU::BVS (void)
{
    if (SR & static_cast <byte> (Flag::V))
    {
        // branch taken cycles added
        ++current.cycles;
        
        current.address += PC;

        // page boundry crossed
        if ((current.address & 0xFF00) != (PC & 0xFF00))
            ++current.cycles;

        PC = current.address;
    }
}

// set interrupt disable
void MOS_6502::CPU::SEI (void)
{
    set_flag (Flag::I, true);
}

void MOS_6502::CPU::STA (void)
{
    write (current.address, AC);
}

// store YR
void MOS_6502::CPU::STY (void)
{
    write (current.address, YR);
}

// store XR
void MOS_6502::CPU::STX (void)
{
    write (current.address, XR);
}

// decrement YR
void MOS_6502::CPU::DEY (void)
{
    --YR;
    set_flag (Flag::Z, YR == 0x00);
    set_flag (Flag::N, YR & 0x80);
}

// transfer XR to accumulator
void MOS_6502::CPU::TXA (void)
{
    AC = XR;
    set_flag (Flag::Z, AC == 0x00);
    set_flag (Flag::N, AC & 0x80);
}

// branch if carry clear
void MOS_6502::CPU::BCC (void)
{
    if (!(SR & static_cast <byte> (Flag::C)))
    {
        // branch taken
        ++current.cycles;

        current.address += PC;

        // page boundry crossed
        if ((current.address & 0xFF00) != (PC & 0xFF00))
            ++current.cycles;

        PC = current.address;
    }
}

// transfer YR to accumulator
void MOS_6502::CPU::TYA (void)
{
    AC = YR;
    set_flag (Flag::Z, AC == 0x00);
    set_flag (Flag::N, AC & 0x80);
}

// transfer XR to stack pointer
void MOS_6502::CPU::TXS (void)
{
    SP = XR;
}

// load YR
void MOS_6502::CPU::LDY (void)
{
    YR = read (current.address);
    set_flag (Flag::Z, YR == 0x00);
    set_flag (Flag::N, YR & 0x80);
}

// load accumulator
void MOS_6502::CPU::LDA (void)
{
    AC = read (current.address);
    set_flag (Flag::Z, AC == 0x00);
    set_flag (Flag::N, AC & 0x80);
}

// load XR
void MOS_6502::CPU::LDX (void)
{
    XR = read (current.address);
    set_flag (Flag::Z, XR == 0x00);
    set_flag (Flag::N, XR & 0x80);
}

// transfer accumulator to YR
void MOS_6502::CPU::TAY (void)
{
    YR = AC;
    set_flag (Flag::Z, YR == 0x00);
    set_flag (Flag::N, YR & 0x80);
}

// transfer accumulator to XR
void MOS_6502::CPU::TAX (void)
{
    XR = AC;
    set_flag (Flag::Z, XR == 0x00);
    set_flag (Flag::N, XR & 0x80);
}

// branch if carry set
void MOS_6502::CPU::BCS (void)
{
    if (SR & static_cast <byte> (Flag::C))
    {
        // branch taken cycles added
        ++current.cycles;
        
        current.address += PC;

        // page boundry crossed
        if ((current.address & 0xFF00) != (PC & 0xFF00))
            ++current.cycles;

        PC = current.address;
    }
}

// clear overflow
void MOS_6502::CPU::CLV (void)
{
    set_flag (Flag::V, false);
}

// transfer stack pointer to XR
void MOS_6502::CPU::TSX (void)
{
    XR = SP;
    set_flag (Flag::Z, XR == 0x00);
    set_flag (Flag::N, XR & 0x80);
}

// compare YR
void MOS_6502::CPU::CPY (void)
{
    current.data = read (current.address);

    set_flag (Flag::C, YR >= current.data);
    set_flag (Flag::Z, YR == 0x00);
    set_flag (Flag::N, (YR - current.data) & 0x80);
}

// compare accumulator
void MOS_6502::CPU::CMP (void)
{
    current.data = read (current.address);

    set_flag (Flag::C, AC >= current.data);
    set_flag (Flag::Z, AC == 0x00);
    set_flag (Flag::N, (AC - current.data) & 0x80);
}

// decrement memory
void MOS_6502::CPU::DEC (void)
{
    current.data = read (current.address);
    
    --current.data;

    set_flag (Flag::Z, current.data == 0x00);
    set_flag (Flag::N, current.data & 0x80);

    write (current.address, current.data);
}

// increment YR
void MOS_6502::CPU::INY (void)
{
    ++YR;
    
    set_flag (Flag::Z, YR == 0x0);
    set_flag (Flag::N, YR & 0x80);
}

// decrement XR
void MOS_6502::CPU::DEX (void)
{
    --XR;
    
    set_flag (Flag::Z, XR == 0x0);
    set_flag (Flag::N, XR & 0x80);
}

// branch if not equal
void MOS_6502::CPU::BNE (void)
{
    if (!(SR & static_cast <byte> (Flag::Z)))
    {
        // branch taken cycles added
        ++current.cycles;
        
        current.address += PC;

        // page boundry crossed
        if ((current.address & 0xFF00) != (PC & 0xFF00))
            ++current.cycles;

        PC = current.address;
    }
}

// clear decimal
void MOS_6502::CPU::CLD (void)
{
    set_flag(Flag::D, false);
}

// compare XR
void MOS_6502::CPU::CPX (void)
{
    current.data = read (current.address);

    set_flag (Flag::C, XR >= current.data);
    set_flag (Flag::Z, XR == current.data);
    set_flag (Flag::N, (XR - current.data) & 0x80);
}

// subtract with carry
// TODO ~(result < 0x00) look at the nes docs
void MOS_6502::CPU::SBC (void)
{
    current.data = read (current.address);

    const word result = AC + ~current.data + (static_cast <byte> (Flag::C) & SR);

    set_flag (Flag::C, !(result < 0x00));
    set_flag (Flag::Z, result == 0x00);
    set_flag (Flag::V, (result ^ AC) & (result ^ ~current.data) & 0x80);
    set_flag (Flag::N, result & 0x80);

    AC = result & 0x00FF;
}

// increment memory
void MOS_6502::CPU::INC (void)
{
    current.data = read (current.address);
    
    ++current.data;
   
    set_flag (Flag::Z, current.data == 0x00);
    set_flag (Flag::N, current.data & 0x80);

    write (current.address, current.data);
}

// increment XR
void MOS_6502::CPU::INX (void)
{
    ++XR;
    
    set_flag (Flag::Z, XR == 0x00);
    set_flag (Flag::N, XR & 0x80);
}

void MOS_6502::CPU::NOP (void)
{}

void MOS_6502::CPU::BEQ (void)
{
    if (SR & static_cast <byte> (Flag::Z))
    {
        // branch taken cycles added
        ++current.cycles;
        
        current.address += PC;

        // page boundry crossed
        if ((current.address & 0xFF00) != (PC & 0xFF00))
            ++current.cycles;

        PC = current.address;
    }
}

// set decimal
void MOS_6502::CPU::SED (void)
{
    set_flag (Flag::D, true);
}

// empty instruction (illegal)
void MOS_6502::CPU::___ (void)
{

}

/* GETTERS */
word MOS_6502::CPU::get_PC () const {return PC;}
byte MOS_6502::CPU::get_AC () const {return AC;}
byte MOS_6502::CPU::get_XR () const {return XR;}
byte MOS_6502::CPU::get_YR () const {return YR;}
byte MOS_6502::CPU::get_SR () const {return SR;}
byte MOS_6502::CPU::get_SP () const {return SP;}
const MOS_6502::Current& MOS_6502::CPU::get_current () const {return current;}
const std::array<MOS_6502::Instruction, 256>& MOS_6502::CPU::get_instruction_table () {return instruction_table;}


MOS_6502::CPU_Trace::CPU_Trace (MOS_6502::CPU& _cpu, std::array <std::uint8_t, 65534>& _memory)
: cpu {_cpu}
, memory {_memory}
{}

void MOS_6502::CPU_Trace::trace ()
{
    std::string line {};
    const auto& current = cpu.get_current();
    MOS_6502::disassemble_line(line, memory, current.pc);
    traces.emplace_back
    (
        cpu.get_PC(),
        cpu.get_AC(),
        cpu.get_XR(),
        cpu.get_YR(),
        cpu.get_SR(),
        cpu.get_SP(),
        std::move(line)
    );
}

const MOS_6502::Trace& MOS_6502::CPU_Trace::operator [] (std::uint16_t index) const
{
    return traces.at(index);
}

std::vector <std::string> MOS_6502::disassembler (const std::array <std::uint8_t, 65534>& memory, std::uint16_t offset)
{
    std::vector <std::string> result {};
    std::uint16_t rom_index = offset;

    while (rom_index < memory.size())
    {
        std::string line{};
        rom_index = disassemble_line(line, memory, rom_index);
        result.push_back (std::move(line));
    }
    return result;
}

std::uint16_t MOS_6502::disassemble_line (std::string& result, const std::array <std::uint8_t, 65534>& memory, std::uint16_t rom_index)
{
    const auto&         ins      = MOS_6502::CPU::instruction_table[static_cast<std::size_t>(memory[rom_index])];
    const std::uint16_t index    = rom_index;
    const char*         mnemonic = MOS_6502::mnemonic_map.at(ins.mnemonic);
    const std::uint8_t  b0       = memory[rom_index];
    const std::uint8_t  b1       = rom_index + 1 < memory.size() ? memory[rom_index+1] : 0;
    const std::uint8_t  b2       = rom_index + 2 < memory.size() ? memory[rom_index+2] : 0;
    
    switch (ins.addr_mode)
    {
        case MOS_6502::Mode::IMP:
        case MOS_6502::Mode::ACC:
            result = std::format ("{:04X}: {:02X} {:>9}", index, b0, mnemonic);
            rom_index += 1;
            break;
        case MOS_6502::Mode::ABS:
            result =  std::format ("{:04X}: {:02X} {:02X} {:02X} {:} ${:04X}", index, b0, b1, b2, mnemonic, (b2 << 8 | b1));
            rom_index += 3;
            break;
        case MOS_6502::Mode::ABX:
            result = std::format ("{:04X}: {:02X} {:02X} {:02X} {:} ${:04X},X", index, b0, b1, b2, mnemonic, (b2 << 8 | b1));
            rom_index += 3;
            break;
        case MOS_6502::Mode::ABY:
            result = std::format ("{:04X}: {:02X} {:02X} {:02X} {:} ${:04X},Y", index, b0, b1, b2, mnemonic, (b2 << 8 | b1));
            rom_index += 3;
            break;
        case MOS_6502::Mode::IMM:
            result = std::format ("{:04X}: {:02X} {:02X} {:>6} #${:02X}", index, b0, b1, mnemonic, b1);
            rom_index += 2;
            break;
        case MOS_6502::Mode::IND:
            result = std::format ("{:04X}: {:02X} {:02X} {:02X} {:s} (${:04X})", index, b0, b1, b2, mnemonic, (b2 << 8) | b1);
            rom_index += 3;
            break;
        case MOS_6502::Mode::XIZ:
            result = std::format ("{:04X}: {:02X} {:02X} {:>6} (${:02X},X)", index, b0, b1, mnemonic, b1);
            rom_index += 2;
            break;
        case MOS_6502::Mode::YIZ:
            result = std::format ("{:04X}: {:02X} {:02X} {:>6} (${:02X}),Y", index, b0, b1, mnemonic, b1);
            rom_index += 2;
            break;
        case MOS_6502::Mode::REL:
            result = std::format ("{:04X}: {:02X} {:02X} {:>6} ${:04X}", index, b0, b1, mnemonic, (index+2) + b1);
            rom_index += 2;
            break;
        case MOS_6502::Mode::ZPG:
            result = std::format ("{:04X}: {:02X} {:02X} {:>6} ${:02X}", index, b0, b1, mnemonic, b1);
            rom_index += 2;
            break;
        case MOS_6502::Mode::ZPX:
            result = std::format ("{:04X}: {:02X} {:02X} {:>6} ${:02X},X", index, b0, b1, mnemonic, b1);
            rom_index += 2;
            break;
        case MOS_6502::Mode::ZPY:
            result = std::format ("{:04X}: {:02X} {:02X} {:>6} ${:02X},Y", index, b0, b1, mnemonic, b1);
            rom_index += 2;
            break;
    }
    return rom_index;
}