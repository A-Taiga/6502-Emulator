#ifndef MOS_6502_H
#define MOS_6502_H

#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <unordered_map>


/*

https://www.nesdev.org/wiki/Instruction_reference

https://www.masswerk.at/6502/6502_instruction_set.html#BVC

data is stored in little-endien (LLHH)

PC	program counter	(16 bit)
AC	accumulator	(8 bit)
X	X register	(8 bit)
Y	Y register	(8 bit)
SR	status register [NV-BDIZC]	(8 bit)
SP	stack pointer	(8 bit)

*/

using byte = std::uint8_t;
using word = std::uint16_t;


// namespace Memory{class ROM;}


namespace MOS_6502
{

    static constexpr word stk_begin         = 0x0100;
    static constexpr word reset_vector_low  = 0xFFFC;
    static constexpr word reset_vector_high = 0xFFFD;
    static constexpr word irq_vector_low    = 0xFFFE;
    static constexpr word irq_vector_high   = 0xFFFF;

    enum class Mnemonic
    {
        BRK, ORA, ASL, PHP, BPL,
        CLC, JSR, AND, BIT, ROL, 
        PLP, BMI, SEC, RTI, EOR,
        LSR, PHA, JMP, BVC, CLI,
        RTS, PLA, ADC, ROR, BVS,
        SEI, STA, STY, STX, DEY,
        TXA, BCC, TYA, TXS, LDY, 
        LDA, LDX, TAY, TAX, BCS, 
        CLV, TSX, CPY, CMP, DEC, 
        INY, DEX, BNE, CLD, CPX, 
        SBC, INC, INX, NOP, BEQ, 
        SED, ___,
    };

    enum class Mode
    {
        ACC,ABS,ABX,ABY,IMM,IMP,IND,XIZ,YIZ,REL,ZPG,ZPX,ZPY,
    };

    enum class Flag: byte
    {
        N = 1 << 7, // negative
        V = 1 << 6, // overflow
        _ = 1 << 5, // ignored / unused
        B = 1 << 4, // break
        D = 1 << 3, // decimal
        I = 1 << 2, // interrupt (IRQ dissabl)
        Z = 1 << 1, // zero
        C = 1 << 0, // carry
    };

    class CPU;
    struct Instruction
    {
        Mnemonic mnemonic;
        Mode     addr_mode;

        void (CPU::*opcode) (void);
        void (CPU::*mode)   (void);
        int  cycle_count;
    };

    struct Current
    {
        Instruction const* instruction;
        word address;
        byte data;
        int cycles;
    };

    inline const std::unordered_map <Mnemonic, const char*> mnemonic_map = 
    {
        {Mnemonic::BRK, "BRK"}, {Mnemonic::ORA, "ORA"}, {Mnemonic::ASL, "ASL"}, {Mnemonic::PHP, "PHP"}, {Mnemonic::BPL, "BPL"},
        {Mnemonic::CLC, "CLC"}, {Mnemonic::JSR, "JSR"}, {Mnemonic::AND, "AND"}, {Mnemonic::BIT, "BIT"}, {Mnemonic::ROL, "ROL"}, 
        {Mnemonic::PLP, "PLP"}, {Mnemonic::BMI, "BMI"}, {Mnemonic::SEC, "SEC"}, {Mnemonic::RTI, "RTI"}, {Mnemonic::EOR, "EOR"},
        {Mnemonic::LSR, "LSR"}, {Mnemonic::PHA, "PHA"}, {Mnemonic::JMP, "JMP"}, {Mnemonic::BVC, "BVC"}, {Mnemonic::CLI, "CLI"},
        {Mnemonic::RTS, "RTS"}, {Mnemonic::PLA, "PLA"}, {Mnemonic::ADC, "ADC"}, {Mnemonic::ROR, "ROR"}, {Mnemonic::BVS, "BVS"},
        {Mnemonic::SEI, "SEI"}, {Mnemonic::STA, "STA"}, {Mnemonic::STY, "STY"}, {Mnemonic::STX, "STX"}, {Mnemonic::DEY, "DEY"},
        {Mnemonic::TXA, "TXA"}, {Mnemonic::BCC, "BCC"}, {Mnemonic::TYA, "TYA"}, {Mnemonic::TXS, "TXS"}, {Mnemonic::LDY, "LDY"}, 
        {Mnemonic::LDA, "LDA"}, {Mnemonic::LDX, "LDX"}, {Mnemonic::TAY, "TAY"}, {Mnemonic::TAX, "TAX"}, {Mnemonic::BCS, "BCS"}, 
        {Mnemonic::CLV, "CLV"}, {Mnemonic::TSX, "TSX"}, {Mnemonic::CPY, "CPY"}, {Mnemonic::CMP, "CMP"}, {Mnemonic::DEC, "DEC"}, 
        {Mnemonic::INY, "INY"}, {Mnemonic::DEX, "DEX"}, {Mnemonic::BNE, "BNE"}, {Mnemonic::CLD, "CLD"}, {Mnemonic::CPX, "CPX"}, 
        {Mnemonic::SBC, "SBC"}, {Mnemonic::INC, "INC"}, {Mnemonic::INX, "INX"}, {Mnemonic::NOP, "NOP"}, {Mnemonic::BEQ, "BEQ"}, 
        {Mnemonic::SED, "SED"}, {Mnemonic::___, "___"},
    };

    inline const std::unordered_map <Mode, const char*> mode_map =
    {
        {Mode::ACC, "ACC"},
        {Mode::ABS, "ABS"},
        {Mode::ABX, "ABX"},
        {Mode::ABY, "ABY"},
        {Mode::IMM, "IMM"},
        {Mode::IMP, "IMP"},
        {Mode::IND, "IND"},
        {Mode::XIZ, "XIZ"},
        {Mode::YIZ, "YIZ"},
        {Mode::REL, "REL"},
        {Mode::ZPG, "ZPG"},
        {Mode::ZPX, "ZPX"},
        {Mode::ZPY, "ZPY"},
    };

    class CPU
    {
    public:
        using read_cb = std::function <byte(const word)>;
        using write_cb = std::function <void(const word, const byte)>;
        
        CPU (read_cb, write_cb);


        void IRQ (void);
        void NMI (void);

        int update (void);
        void reset (void);

        bool check_flag (Flag flag) const;



        word old_PC; // for tracing


    private:

        read_cb  read;
        write_cb write;

        word PC;    // program counter
        byte AC;    // accumulator
        byte XR;    // x register
        byte YR;    // y register
        byte SR;    // status register
        byte SP;    // stack pointer

 

        void set_flag   (const Flag, const bool);
        void stack_push (const byte val);
        byte stack_pop  (void);


        struct Current current;

        /* OPCODES */
        void BRK (void); void ORA (void); void ASL (void); void PHP (void); void BPL (void);
        void CLC (void); void JSR (void); void AND (void); void BIT (void); void ROL (void); 
        void PLP (void); void BMI (void); void SEC (void); void RTI (void); void EOR (void);
        void LSR (void); void PHA (void); void JMP (void); void BVC (void); void CLI (void);
        void RTS (void); void PLA (void); void ADC (void); void ROR (void); void BVS (void);
        void SEI (void); void STA (void); void STY (void); void STX (void); void DEY (void);
        void TXA (void); void BCC (void); void TYA (void); void TXS (void); void LDY (void); 
        void LDA (void); void LDX (void); void TAY (void); void TAX (void); void BCS (void); 
        void CLV (void); void TSX (void); void CPY (void); void CMP (void); void DEC (void); 
        void INY (void); void DEX (void); void BNE (void); void CLD (void); void CPX (void); 
        void SBC (void); void INC (void); void INX (void); void NOP (void); void BEQ (void); 
        void SED (void); void ___ (void); // ___ = illegal

        /* ADDRESSING MODES */
        void ACC (void); // accumulator 
        void ABS (void); // absolute
        void ABX (void); // absoulte X-indexed
        void ABY (void); // absolute Y-indexed
        void IMM (void); // immediate
        void IMP (void); // implied
        void IND (void); // indirect
        void XIZ (void); // X-indexed indirect zeropage address
        void YIZ (void); // Y-indexed indirect zeropage address
        void REL (void); // relative
        void ZPG (void); // zeropage
        void ZPX (void); // zeropage X-indexed
        void ZPY (void); // zeropage Y-indexed

        using _ = CPU;
        using M = Mnemonic;
        using A = Mode;
        
    public:

        static constexpr std::array<Instruction, 256> instruction_table
        {{
            {M::BRK, A::IMP, &_::BRK, &_::IMP, 7}, {M::ORA, A::XIZ, &_::ORA, &_::XIZ, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::ORA, A::ZPG, &_::ORA, &_::ZPG, 3}, {M::ASL, A::ZPG, &_::ASL, &_::ZPG, 5}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::PHP, A::IMP, &_::PHP, &_::IMP, 3}, {M::ORA, A::IMM, &_::ORA, &_::IMM, 2}, {M::ASL, A::ACC, &_::ASL, &_::ACC, 2}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::ORA, A::ABS, &_::ORA, &_::ABS, 4}, {M::ASL, A::ABS, &_::ASL, &_::ABS, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0}, 
            {M::BPL, A::REL, &_::BPL, &_::REL, 2}, {M::ORA, A::YIZ, &_::ORA, &_::YIZ, 5}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::ORA, A::ZPX, &_::ORA, &_::ZPX, 4}, {M::ASL, A::ZPX, &_::ASL, &_::ZPX, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::CLC, A::IMP, &_::CLC, &_::IMP, 2}, {M::ORA, A::ABY, &_::ORA, &_::ABY, 4}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::ORA, A::ABX, &_::ORA, &_::ABX, 4}, {M::ASL, A::ABX, &_::ASL, &_::ABX, 7}, {M::___, A::IMP, &_::___, &_::IMP, 0}, 
            {M::JSR, A::ABS, &_::JSR, &_::ABS, 6}, {M::AND, A::XIZ, &_::AND, &_::XIZ, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::BIT, A::ZPG, &_::BIT, &_::ZPG, 3}, {M::AND, A::ZPG, &_::AND, &_::ZPG, 3}, {M::ROL, A::ZPG, &_::ROL, &_::ZPG, 5}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::PLP, A::IMP, &_::PLP, &_::IMP, 4}, {M::AND, A::IMM, &_::AND, &_::IMM, 2}, {M::ROL, A::ACC, &_::ROL, &_::ACC, 2}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::BIT, A::ABS, &_::BIT, &_::ABS, 4}, {M::AND, A::ABS, &_::AND, &_::ABS, 4}, {M::ROL, A::ABS, &_::ROL, &_::ABS, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0}, 
            {M::BMI, A::REL, &_::BMI, &_::REL, 2}, {M::AND, A::YIZ, &_::AND, &_::YIZ, 5}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::AND, A::ZPX, &_::AND, &_::ZPX, 4}, {M::ROL, A::ZPX, &_::ROL, &_::ZPX, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::SEC, A::IMP, &_::SEC, &_::IMP, 2}, {M::AND, A::ABY, &_::AND, &_::ABY, 4}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::AND, A::ABX, &_::AND, &_::ABX, 4}, {M::ROL, A::ABX, &_::ROL, &_::ABX, 7}, {M::___, A::IMP, &_::___, &_::IMP, 0}, 
            {M::RTI, A::IMP, &_::RTI, &_::IMP, 6}, {M::EOR, A::XIZ, &_::EOR, &_::XIZ, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::EOR, A::ZPG, &_::EOR, &_::ZPG, 3}, {M::LSR, A::ZPG, &_::LSR, &_::ZPG, 5}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::PHA, A::IMP, &_::PHA, &_::IMP, 3}, {M::EOR, A::IMM, &_::EOR, &_::IMM, 2}, {M::LSR, A::ACC, &_::LSR, &_::ACC, 2}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::JMP, A::ABS, &_::JMP, &_::ABS, 3}, {M::EOR, A::ABS, &_::EOR, &_::ABS, 4}, {M::LSR, A::ABS, &_::LSR, &_::ABS, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0}, 
            {M::BVC, A::REL, &_::BVC, &_::REL, 2}, {M::EOR, A::YIZ, &_::EOR, &_::YIZ, 5}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::EOR, A::ZPX, &_::EOR, &_::ZPX, 4}, {M::LSR, A::ZPX, &_::LSR, &_::ZPX, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::CLI, A::IMP, &_::CLI, &_::IMP, 2}, {M::EOR, A::ABY, &_::EOR, &_::ABY, 4}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::EOR, A::ABX, &_::EOR, &_::ABX, 4}, {M::LSR, A::ABX, &_::LSR, &_::ABX, 7}, {M::___, A::IMP, &_::___, &_::IMP, 0}, 
            {M::RTS, A::IMP, &_::RTS, &_::IMP, 6}, {M::ADC, A::XIZ, &_::ADC, &_::XIZ, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::ADC, A::ZPG, &_::ADC, &_::ZPG, 3}, {M::ROR, A::ZPG, &_::ROR, &_::ZPG, 5}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::PLA, A::IMP, &_::PLA, &_::IMP, 4}, {M::ADC, A::IMM, &_::ADC, &_::IMM, 2}, {M::ROR, A::ACC, &_::ROR, &_::ACC, 2}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::JMP, A::IND, &_::JMP, &_::IND, 5}, {M::ADC, A::ABS, &_::ADC, &_::ABS, 4}, {M::ROR, A::ABS, &_::ROR, &_::ABS, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0}, 
            {M::BVS, A::REL, &_::BVS, &_::REL, 2}, {M::ADC, A::YIZ, &_::ADC, &_::YIZ, 5}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::ADC, A::ZPX, &_::ADC, &_::ZPX, 4}, {M::ROR, A::ZPX, &_::ROR, &_::ZPX, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::SEI, A::IMP, &_::SEI, &_::IMP, 2}, {M::ADC, A::ABY, &_::ADC, &_::ABY, 4}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::ADC, A::ABX, &_::ADC, &_::ABX, 4}, {M::ROR, A::ABX, &_::ROR, &_::ABX, 7}, {M::___, A::IMP, &_::___, &_::IMP, 0}, 
            {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::STA, A::XIZ, &_::STA, &_::XIZ, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::STY, A::ZPG, &_::STY, &_::ZPG, 3}, {M::STA, A::ZPG, &_::STA, &_::ZPG, 3}, {M::STX, A::ZPG, &_::STX, &_::ZPG, 3}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::DEY, A::IMP, &_::DEY, &_::IMP, 2}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::TXA, A::IMP, &_::TXA, &_::IMP, 2}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::STY, A::ABS, &_::STY, &_::ABS, 4}, {M::STA, A::ABS, &_::STA, &_::ABS, 4}, {M::STX, A::ABS, &_::STX, &_::ABS, 4}, {M::___, A::IMP, &_::___, &_::IMP, 0}, 
            {M::BCC, A::REL, &_::BCC, &_::REL, 2}, {M::STA, A::YIZ, &_::STA, &_::YIZ, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::STY, A::ZPX, &_::STY, &_::ZPX, 4}, {M::STA, A::ZPX, &_::STA, &_::ZPX, 4}, {M::STX, A::ZPY, &_::STX, &_::ZPY, 4}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::TYA, A::IMP, &_::TYA, &_::IMP, 2}, {M::STA, A::ABY, &_::STA, &_::ABY, 5}, {M::TXS, A::IMP, &_::TXS, &_::IMP, 2}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::STA, A::ABX, &_::STA, &_::ABX, 5}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, 
            {M::LDY, A::IMM, &_::LDY, &_::IMM, 2}, {M::LDA, A::XIZ, &_::LDA, &_::XIZ, 6}, {M::LDX, A::IMM, &_::LDX, &_::IMM, 2}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::LDY, A::ZPG, &_::LDY, &_::ZPG, 3}, {M::LDA, A::ZPG, &_::LDA, &_::ZPG, 3}, {M::LDX, A::ZPG, &_::LDX, &_::ZPG, 3}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::TAY, A::IMP, &_::TAY, &_::IMP, 2}, {M::LDA, A::IMM, &_::LDA, &_::IMM, 2}, {M::TAX, A::IMP, &_::TAX, &_::IMP, 2}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::LDY, A::ABS, &_::LDY, &_::ABS, 4}, {M::LDA, A::ABS, &_::LDA, &_::ABS, 4}, {M::LDX, A::ABS, &_::LDX, &_::ABS, 4}, {M::___, A::IMP, &_::___, &_::IMP, 0}, 
            {M::BCS, A::REL, &_::BCS, &_::REL, 2}, {M::LDA, A::YIZ, &_::LDA, &_::YIZ, 5}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::LDY, A::ZPX, &_::LDY, &_::ZPX, 4}, {M::LDA, A::ZPX, &_::LDA, &_::ZPX, 4}, {M::LDX, A::ZPY, &_::LDX, &_::ZPY, 4}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::CLV, A::IMP, &_::CLV, &_::IMP, 2}, {M::LDA, A::ABY, &_::LDA, &_::ABY, 4}, {M::TSX, A::IMP, &_::TSX, &_::IMP, 2}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::LDY, A::ABX, &_::LDY, &_::ABX, 4}, {M::LDA, A::ABX, &_::LDA, &_::ABX, 4}, {M::LDX, A::ABY, &_::LDX, &_::ABY, 4}, {M::___, A::IMP, &_::___, &_::IMP, 0}, 
            {M::CPY, A::IMM, &_::CPY, &_::IMM, 2}, {M::CMP, A::XIZ, &_::CMP, &_::XIZ, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::CPY, A::ZPG, &_::CPY, &_::ZPG, 3}, {M::CMP, A::ZPG, &_::CMP, &_::ZPG, 3}, {M::DEC, A::ZPG, &_::DEC, &_::ZPG, 5}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::INY, A::IMP, &_::INY, &_::IMP, 2}, {M::CMP, A::IMM, &_::CMP, &_::IMM, 2}, {M::DEX, A::IMP, &_::DEX, &_::IMP, 2}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::CPY, A::ABS, &_::CPY, &_::ABS, 4}, {M::CMP, A::ABS, &_::CMP, &_::ABS, 4}, {M::DEC, A::ABS, &_::DEC, &_::ABS, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0}, 
            {M::BNE, A::REL, &_::BNE, &_::REL, 2}, {M::CMP, A::YIZ, &_::CMP, &_::YIZ, 5}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::CMP, A::ZPX, &_::CMP, &_::ZPX, 4}, {M::DEC, A::ZPX, &_::DEC, &_::ZPX, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::CLD, A::IMP, &_::CLD, &_::IMP, 2}, {M::CMP, A::ABY, &_::CMP, &_::ABY, 4}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::CMP, A::ABX, &_::CMP, &_::ABX, 4}, {M::DEC, A::ABX, &_::DEC, &_::ABX, 7}, {M::___, A::IMP, &_::___, &_::IMP, 0}, 
            {M::CPX, A::IMM, &_::CPX, &_::IMM, 2}, {M::SBC, A::XIZ, &_::SBC, &_::XIZ, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::CPX, A::ZPG, &_::CPX, &_::ZPG, 3}, {M::SBC, A::ZPG, &_::SBC, &_::ZPG, 3}, {M::INC, A::ZPG, &_::INC, &_::ZPG, 5}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::INX, A::IMP, &_::INX, &_::IMP, 2}, {M::SBC, A::IMM, &_::SBC, &_::IMM, 2}, {M::NOP, A::IMP, &_::NOP, &_::IMP, 2}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::CPX, A::ABS, &_::CPX, &_::ABS, 4}, {M::SBC, A::ABS, &_::SBC, &_::ABS, 4}, {M::INC, A::ABS, &_::INC, &_::ABS, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0},
            {M::BEQ, A::REL, &_::BEQ, &_::REL, 2}, {M::SBC, A::YIZ, &_::SBC, &_::YIZ, 5}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::SBC, A::ZPX, &_::SBC, &_::ZPX, 4}, {M::INC, A::ZPX, &_::INC, &_::ZPX, 6}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::SED, A::IMP, &_::SED, &_::IMP, 2}, {M::SBC, A::ABY, &_::SBC, &_::ABY, 5}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::___, A::IMP, &_::___, &_::IMP, 0}, {M::SBC, A::ABX, &_::SBC, &_::ABX, 4}, {M::INC, A::ABX, &_::INC, &_::ABX, 7}, {M::___, A::IMP, &_::___, &_::IMP, 0},
        }};
    
        /* GETTERS */
        word get_PC () const;
        byte get_AC () const;
        byte get_XR () const;
        byte get_YR () const;
        byte get_SR () const;
        byte get_SP () const;
        const Current& get_current () const;
        static const std::array<Instruction, 256>& get_instruction_table ();
    };
}




namespace MOS_6502
{
    using line_type     = std::tuple<std::uint16_t, std::string, bool>;
    using trace_type    = std::vector<std::vector<std::string>>;
    using code_map_type = std::unordered_map <std::size_t, const line_type&>;

    std::vector <line_type> disassemble (const std::span<std::uint8_t>& rom, std::uint16_t offset = 0);
    std::uint16_t           disassemble_line (line_type& result, const std::span<std::uint8_t>& rom, std::uint16_t rom_index);
    code_map_type           code_mapper (const std::vector<line_type>& code);
    bool                    trace (trace_type& traces, const code_map_type& map, const MOS_6502::CPU &  cpu);
}

#endif