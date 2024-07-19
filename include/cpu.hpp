#ifndef CPU_HPP
#define CPU_HPP

#include "common.hpp"
#include <string>
#include <vector>

/*
    https://www.masswerk.at/6502/6502_opcode_set.html#BVS

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
    F	Break
    D	Decimal (use BCD for arithmetics)
    I	Interrupt (IRQ disable)
    Z	Zero
    C	
    Processor Stack
    LIFO, top-down, 8 bit range, 0x0100 - 0x01FF
*/


namespace _6502
{
    using flag_type = std::uint8_t;
    enum class FLAG : flag_type
    {
        C = 1 << 0, // carry
        Z = 1 << 1, // zero 
        I = 1 << 2, // interrupt
        D = 1 << 3, // decimal
        B = 1 << 4, // break
        U = 1 << 5, // ignored / unused
        V = 1 << 6, // overflow
        N = 1 << 7, // negative
    };

    enum class Address_Type
    {
        IMP, IMM, ABS, ZPG, ABX, ABY,
        ZPX, ZPY, IND, IZX, IZY, REL
    };

    class CPU;
    struct opcode
    {
        const char* mnemonic;
        void (_6502::CPU::*op)(void);
        int (_6502::CPU::*mode)(void);
        Address_Type addrType;
        std::size_t cycles;
    };

    class Bus;
    class CPU
    {
        private:



            byte read       (const word address);
            void write      (const word address, const byte data);
            void stack_push (const byte data);
            byte stack_pop  ();
            void set_flag   (const FLAG flag, const bool condition);

        public:

            word PC;   /* program counter */
            byte AC;   /* accumulator */
            byte X;    /* x register */
            byte Y;    /* y register */
            byte SR;   /* status register [NV-BDIZC] (aka flags) */
            byte SP;   /* stack pointer */


            CPU             (Bus& bus);
            void reset      ();
            void IRQ        ();
            void NMI        ();
            void decompiler ();
            void run        ();

            // short IF (const byte& op); // opcode fetch returns opcode size
            /* main registers */

            Bus& bus;
            std::vector<std::pair<word, std::string>>  decompiledCode;

            struct
            {
                const opcode* fetched;
                word data;
                int cycles;

            } ins;

            // fix how we get these 

            int IMP (); 
            int IMM (); 
            int ABS (); 
            int ZPG (); 
            int ABX (); 
            int ABY ();
            int ZPX (); 
            int ZPY (); 
            int IND (); 
            int IZX (); 
            int IZY (); 
            int REL ();

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
            void SED (void); void XXX (void);



        };
}

#endif