#ifndef CPU_HPP
#define CPU_HPP

#include <cstdint>
#include <array>
/*
    https://www.masswerk.at/6502/6502_instruction_set.html#BVS

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


using byte = std::uint8_t;
using word = std::uint16_t;


[[maybe_unused]] constexpr byte C = 1 << 0;
[[maybe_unused]] constexpr byte Z = 1 << 1;
[[maybe_unused]] constexpr byte I = 1 << 2;
[[maybe_unused]] constexpr byte D = 1 << 3;
[[maybe_unused]] constexpr byte B = 1 << 4;
[[maybe_unused]] constexpr byte V = 1 << 6;
[[maybe_unused]] constexpr byte N = 1 << 7;
[[maybe_unused]] constexpr word ZRO_BEGIN  = 0x0000;
[[maybe_unused]] constexpr word STK_BEGIN  = 0x0100;
[[maybe_unused]] constexpr word ROM_BEGIN  = 0xF000;
[[maybe_unused]] constexpr word ZRO_END    = 0x00FF;
[[maybe_unused]] constexpr word STK_END    = 0x01FF;
[[maybe_unused]] constexpr word ROM_END    = 0xFFFA;

enum class MODE : byte
{
    IMP, // Implied Addressing
    IMM, // Immediate Addressing
    ABS, // Absolute Addressing
    ZPG, // Zero-Page Addressing
    ABX, // Indexed Addressing: Absolute,X
    ABY, // Indexed Addressing: Absolute,Y
    ZPX, // Indexed Addressing: Zero-Page,X
    ZPY, // Indexed Addressing: (Zero-Page,Y)
    IND, // Indirect Addressing
    IZX, // Indirect Zero-page X-indexed
    IZY, // Indirect Zero-page Y-indexed
    REL, // Relative Addressing (Conditional Branching)
};

struct RAM
{
    std::array<std::uint8_t, 65536> mem; // 65kb
    byte& operator[](word index)
    {
        return mem[index];
    }
};

struct _6502;
struct instruction
{
    const char* mnemonic;
    void (_6502::*opcode)(void) = nullptr;
    MODE addressMode;
    std::size_t cycles;
};

struct _6502
{
    /* main registers */
    word PC;   /* program counter */
    byte AC;   /* accumulator */
    byte X;    /* x register */
    byte Y;    /* y register */
    byte SR;   /* status register [NV-BDIZC] (aka flags) */
    byte SP;   /* stack pointer */
    MODE addressMode;
    RAM memory;
    std::array<instruction,256> opcodes;
    _6502(const char* filePath);
    void reset ();
    void decompiler ();
    void run ();
    void BRK (void); void ORA (void); void ASL (void); void PHP (void); void BPL (void);
    void CLC (void); void JSR (void); void AND (void); void BIT (void); void ROL (void); 
    void PLP (void); void BMI (void); void SEC (void); void RTI (void); void EOR (void);
    void LSR (void); void PHA (void); void JMP (void); void BVC (void); void CLI (void);
    void RTS (void); void PLA (void); void ADC (void); void ROR (void); void BVS (void);
    void SEI (void); void STA (void); void STY (void); void STX (void); void DEY (void);
    void TXA (void); void STZ (void); void BBC (void); void TYA (void); void TXS (void);
    void LDY (void); void LDA (void); void LDX (void); void TAY (void); void TAX (void);
    void BCS (void); void CLV (void); void TSX (void); void CPY (void); void CMP (void);
    void DEC (void); void INY (void); void DEX (void); void BNE (void); void CLD (void);
    void CPX (void); void SBC (void); void INC (void); void INX (void); void SPC (void);
    void NOP (void); void BEQ (void); void SED (void); void XXX (void);
};

#endif