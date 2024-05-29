#ifndef CPU_HPP
#define CPU_HPP

#include <cstdint>
#include <array>
#include <string>


/*
    https://en.wikipedia.org/wiki/MOS_Technology_6502

    Like its precursor, the 6800, the 6502 has very few registers. The 6502's registers 
    include one 8-bit accumulator register (A), two 8-bit index registers (X and Y), 7 
    processor status flag bits (P; from bit 7 to bit 0 these are the negative (N), overflow (V), 
    reserved, break (B), decimal (D), interrupt disable (I), zero (Z) and carry (C) flag), an 8-bit stack 
    pointer (S), and a 16-bit program counter (PC).[75] This compares to a contemporaneous competitor, the Intel 
    8080, which likewise has one 8-bit accumulator and a 16-bit program counter, but has six more general-purpose 8-bit 
    registers (which can be combined into three 16-bit pointers), and a larger 16-bit stack pointer
*/


/*
    https://www.masswerk.at/6502/6502_instruction_set.html

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





struct _6502
{

    // main registers
    std::uint16_t PC;   // program counter
    std::uint8_t  AC;   // accumulator
    std::uint8_t  x;    // x register
    std::uint8_t  y;    // y register
    std::uint8_t  SR;   // status register [NV-BDIZC] (aka flags)
    std::uint8_t  SP;   // stack pointer
    std::array<std::uint8_t, 65536> memory; // 256 pages of 256 bits each
    void load_rom(const char* filePath);
    void decompiler();

    struct instruction
    {
        void (_6502::*op)(void);
    };

};


struct opcode
{

};


namespace opcodes
{
    void XXX(void*);        // no opcode

    void BRK_IMPL(void*);   // 0x00
    void ORA_X_IND(void*);  // 0x01
    //
    //
    //
    void ORA_ZPG(void*);    // 0x05
    void ASL_ZPG(void*);    // 0x06
    //
    void PHP_IMPL(void*);   // 0x08
    void ORA_IMM(void*);    // 0x09
    void ASL_A(void*);      // 0x0A
    //
    //
    void ORA_ABS(void*);    // 0x0D
    void ASL_ABS(void*);    // 0x0E
    //
    void BPL_REL(void*);    // 0x10
    void ORA_IND_Y(void*);  // 0x11
    //
    //
    //
    void ORA_ZPG_X(void*);  // 0x15
    void ASL_ZPG_X(void*);  // 0x16
    //
    void CLC_IMPL(void*);   // 0x18
    void ORA_ABS_Y(void*);  // 0x19
    //
    //
    //
    void ORA_ABS_X(void*);  // 0x1D
    void ASL_ABS_X(void*);  // 0x1E
    //
    void JSR_ABS(void*);    // 0x20
    void AND_X_IND(void*);  // 0x21
    //
    //
    void BIT_ZPG(void*);    // 0x24
    void AND_ZPG(void*);    // 0x25
    void ROL_ZPG(void*);    // 0x26
    //
    void PLP_IMPL(void*);   // 0x28
    void AND_IMM(void*);    // 0x29
    void ROL_A(void*);      // 0x2A
    //
    //
    //
    void BIT_ABS(void*);    // 0x2C
    void AND_ABS(void*);    // 0x2D
    void ROL_ABS(void*);    // 0x2E
    //
    void BMI_REL(void*);    // 0x30
    void AND_IND_Y(void*);  // 0x31
    //
    //
    //
    void AND_ZPG_X(void*);  // 0x35
    void ROL_ZPG_X(void*);  // 0x36
    //
    void SEC_IMPL(void*);   // 0x38
    void AND_ABS_Y(void*);  // 0x39
    //
    //
    //
    void AND_ABS_X(void*);  // 0x3D
    void ROL_ABS_X(void*);  // 0x3E
    //
    void RTI_IMPL(void*);   //0x40
    void EOR_X_IND(void*);  // 0x41
    //
    //
    //
    void EOR_ZPG(void*);    // 0x45
    void LSR_ZPG(void*);    // 0x46
    //
    void PHA_IMPL(void*);   // 0x48
    void EOR_IMM(void*);    // 0x49
    void LSR_A(void*);      // 0x4A
    //
    void JMP_ABS(void*);    // 0x4C
    void EOR_ABS(void*);    // 0x4D
    void LSR_ABS(void*);    // 0x4E
    //
    void BVC_REL(void*);    // 0x50
    void EOR_IND_Y(void*);  // 0x51
    //
    //
    //
    void EOR_ZPG_X(void*);  // 0x55
    void LSR_ZPG_X(void*);  // 0x56
    //
    void CLI_IMPL(void*);   // 0x58
    void EOR_ABS_Y(void*);  // 0x59
    //
    //
    //
    void EOR_ABS_X(void*);  // 0x5D
    void LSR_ABS_X(void*);  // 0x5E
    //
    void RTS_IMPL(void*);   // 0x60
    void ADC_X_IND(void*);  // 0x61
    //
    //
    //
    void ADC_ZPG(void*);    // 0x65
    void ROR_ZPG(void*);    // 0x66
    //
    void PLA_IMPL(void*);   // 0x68
    void ADC_IMM(void*);    // 0x69
    void ROR_A(void*);      // 0x6A
    //
    void JMP_IND(void*);    // 0x6C
    void ADC_ABS(void*);    // 0x6D
    void ROR_ABS(void*);    // 0x6E
    //
    void BVS_REL(void*);    // 0x70
    void ADC_IND_Y(void*);  // 0x71
    //
    //
    //
    void ADC_ZPG_X(void*);  // 0x75
    void ROR_ZPG_X(void*);  // 0x76
    //
    void SEI_IMPL(void*);   // 0x78
    void ADC_ABS_Y(void*);  // 0x79
    //
    //
    //
    void ADC_ABS_X(void*);  // 0x7D
    void ROR_ABS_X(void*);  // 0x7E
    //
    //
    void STA_X_IND(void*);  // 0x81
    //
    //
    void STY_ZPG(void*);    // 0x84
    void STA_ZPG(void*);    // 0x85
    void STX_ZPG(void*);    // 0x86
    //
    void DEY_IMPL(void*);   // 0x88
    //
    void TEX_IMPL(void*);   // 0x8A
    //
    void STY_ABS(void*);    // 0x8C
    void STA_ABS(void*);    // 0x8D
    void STZ_ABS(void*);    // 0x8E
    //
    void BBC_REL(void*);    // 0x90
    void STA_IND_Y(void*);  // 0x91
    //
    //
    void STY_ZPG_X(void*);  // 0x94
    void STA_ZPG_X(void*);  // 0x95
    void STX_ZPG_Y(void*);  // 0x96
    //
    void TYA_IMPL(void*);   // 0x98
    void STA_ABS_Y(void*);  // 0x99
    void TXS_IMPL(void*);   // 0x9A
    //
    //
    void STA_ABS_X(void*);  // 0x9D
    //
    //
    void LDY_IMM(void*);    // 0xA0
    void LDA_X_IND(void*);  // 0xA1
    void LDX_IMM(void*);    // 0xA2
    //
    void LDY_ZPG(void*);    // 0xA4
    void LDA_ZPG(void*);    // 0xA5
    void LDX_ZPG(void*);    // 0xA6
    //
    void TAY_IMPL(void*);   // 0xA8
    void LDA_IMM(void*);    // 0xA9
    void TAX_IMPL(void*);   // 0xAA
    //
    void LDY_ABS(void*);    // 0xAC
    void LDA_ABS(void*);    // 0xAD
    void LDX_ABS(void*);    // 0xAE
    //
    void BCS_REL(void*);    // 0xB0
    void LDA_IND_Y(void*);  // 0xB1
    //
    //
    void LDY_ZPG_X(void*);  // 0xB4
    void LDA_ZPG_X(void*);  // 0xB5
    void LDX_ZPG_Y(void*);  // 0xB6
    //
    void CLV_IMPL(void*);   // 0xB8
    void LDA_ABS_Y(void*);  // 0xB9
    void TSX_IMPL(void*);   // 0xBA
    //
    void LDY_ABS_X(void*);  // 0xBC
    void LDA_ABS_X(void*);  // 0xBD
    void LDX_ABS_Y(void*);  // 0xBE
    //
    void CPY_IMM(void*);    // 0xC0
    void CMP_X_IND(void*);  // 0xC1
    //
    //
    void CPY_ZPG(void*);    // 0xC4
    void CMP_ZPG(void*);    // 0xC5
    void DEC_ZPG(void*);    // 0xC6
    //
    void INY_IMPL(void*);   // 0xC8
    void CMP_IMM(void*);    // 0xC9
    void DEX_IMPL(void*);   // 0xCA
    //
    void CPY_ABS(void*);    // 0xCC
    void CMP_ABS(void*);    // 0xCD
    void DEC_ABS(void*);    // 0xCE
    //
    void BNE_REL(void*);    // 0xD0
    void CMP_IND_Y(void*);  // 0xD1
    //
    //
    //
    void CMP_ZPG_X(void*);  // 0xD5
    void DEC_ZPG_X(void*);  // 0xD6
    //
    void CLD_IMPL(void*);   // 0xD8
    void CMP_ABS_Y(void*);  // 0xD9
    //
    //
    //
    void CMP_ABS_X(void*);  // 0xDD
    void DEC_ABS_X(void*);  // 0xDE
    //
    void CPX_IMM(void*);    // 0xE0
    void SBC_X_IND(void*);  // 0xE1
    //
    //
    void CPX_ZPG(void*);    // 0xE4
    void SBC_ZPG(void*);    // 0xE5
    void INC_ZPG(void*);    // 0xE6
    //
    void INX_IMPL(void*);   // 0xE8
    void SPC_IMM(void*);    // 0xE9
    void NOP_IMPL(void*);   // 0xEA
    //
    void CPX_ABS(void*);    // 0xEC
    void SBC_ABS(void*);    // 0xED
    void INC_ABS(void*);    // 0xEE
    //
    void BEQ_REL(void*);    // 0xF0
    void SBC_IND_Y(void*);  // 0xF1
    //
    //
    //
    void SBC_ZPG_X(void*);  // 0xF5
    void INC_ZPG_X(void*);  // 0xF6
    //
    void SED_IMPL(void*);   // 0xF8
    void SBC_ABS_Y(void*);  // 0xF9
    //
    //
    //
    void SBC_ABS_X(void*);  // 0xFD
    void INC_ABS_X(void*);  // 0xFE
    //








}



#endif

