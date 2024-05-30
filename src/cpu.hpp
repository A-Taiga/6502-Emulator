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

enum class MODE : std::uint8_t
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
    PZX, // Pre-Indexed Indirect, "(Zero-Page,X)"
    PZY, // Post-Indexed Indirect, "(Zero-Page),Y"
    REL, // Relative Addressing (Conditional Branching)
};

struct _6502
{

    /* main registers */
    std::uint16_t PC;   /* program counter */
    std::uint8_t  AC;   /* accumulator */
    std::uint8_t  x;    /* x register */
    std::uint8_t  y;    /* y register */
    std::uint8_t  SR;   /* status register [NV-BDIZC] (aka flags) */
    std::uint8_t  SP;   /* stack pointer */
    std::array<std::uint8_t, 65536> memory; /* 256 pages of 256 bits each */
    struct instruction
    {
        const char* mnemonic;
        void (_6502::*opcode)(void) = nullptr;
        MODE addressMode;
        std::uint8_t cycles;

    };
    std::array<instruction,256> opcodes;
    void load_rom(const char* filePath);
    void decompiler();
    _6502();

    void XXX(void); 
    void BRK(void);
    void ORA(void);
    void ASL(void);
    void PHP(void);
    void BPL(void);
    void CLC(void);
    void JSR(void);
    void AND(void);
    void BIT(void);
    void ROL(void);
    void PLP(void);
    void BMI(void);
    void SEC(void);
    void RTI(void);
    void EOR(void);
    void LSR(void);
    void PHA(void);
    void JMP(void);
    void BVC(void);
    void CLI(void);
    void RTS(void);
    void PLA(void);
    void ADC(void);
    void ROR(void);
    void BVS(void);
    void SEI(void);
    void STA(void);
    void STY(void);
    void STX(void);
    void DEY(void);
    void TXA(void);
    void STZ(void);
    void BBC(void);
    void TYA(void);
    void TXS(void);
    void LDY(void);
    void LDA(void);
    void LDX(void);
    void TAY(void);
    void TAX(void);
    void BCS(void);
    void CLV(void);
    void TSX(void);
    void CPY(void);
    void CMP(void);
    void DEC(void);
    void INY(void);
    void DEX(void);
    void BNE(void);
    void CLD(void);
    void CPX(void);
    void SBC(void);
    void INC(void);
    void INX(void);
    void SPC(void);
    void NOP(void);
    void BEQ(void);
    void SED(void);
};

#endif

