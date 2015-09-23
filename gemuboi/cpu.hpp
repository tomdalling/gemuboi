#pragma once

#include "types.hpp"

namespace CPU {
    struct Registers {
        union {
            U16 af; // accumulator/flags
            struct { U8 f; U8 a; };
        };
        union {
            U16 bc;
            struct { U8 c; U8 b; };
        };
        union {
            U16 de;
            struct { U8 e; U8 d; };
        };
        union {
            U16 hl;
            struct { U8 l; U8 h; };
        };
        U16 sp; // stack pointer
        U16 pc; // program counter

        /*
         Flag register (F) bits:

            7	6	5	4	3	2	1	0
            Z	N	H	C	0	0	0	0

            Z - Zero Flag
            N - Subtract Flag
            H - Half Carry Flag
            C - Carry Flag
            0 - Not used, always zero
        */
        const U8 FlagMask_Zero = 0x80;
        const U8 FlagMask_Subtract = 0x40;
        const U8 FlagMask_HalfCarry = 0x20;
        const U8 FlagMask_Carry = 0x10;

        BOOL32 flag(U8 flag_mask) const { return ((f & flag_mask) == flag_mask); }
        BOOL32 zero_flag() const { return flag(FlagMask_Zero); }
        BOOL32 subtract_flag() const { return flag(FlagMask_Subtract); }
        BOOL32 halfcarry_flag() const { return flag(FlagMask_HalfCarry); }
        BOOL32 carry_flag() const { return flag(FlagMask_Carry); }

        void set_flag(U8 flag_mask, BOOL32 on){
            if(on) {
                f |= flag_mask;
            } else {
                f &= ~flag_mask;
            }
        }
        void set_zero_flag(BOOL32 on){ set_flag(FlagMask_Zero, on); }
        void set_subtract_flag(BOOL32 on){ set_flag(FlagMask_Subtract, on); }
        void set_halfcarry_flag(BOOL32 on){ set_flag(FlagMask_HalfCarry, on); }
        void set_carry_flag(BOOL32 on){ set_flag(FlagMask_Carry, on); }
    };

    struct OpcodeDesc {
        U8 byte_length; // length of opcode plus operands
        U8 cycles; // MINIMUM cycles taken. can be more than this
        const char* description; //pseudo ASM
    };

    const OpcodeDesc Opcodes[256] = {
        /*
         Direct value meanings:

            d8  means immediate 8 bit data
            d16 means immediate 16 bit data
            a8  means 8 bit unsigned data, which are added to $FF00 in certain instructions (replacement for missing IN and OUT instructions)
            a16 means 16 bit address
            r8  means 8 bit signed data, which are added to program counter

         Pseudo ASM alternative meanings:

            LD A,(C) has alternative mnemonic LD A,($FF00+C)
            LD C,(A) has alternative mnemonic LD ($FF00+C),A
            LDH A,(a8) has alternative mnemonic LD A,($FF00+a8)
            LDH (a8),A has alternative mnemonic LD ($FF00+a8),A
            LD A,(HL+) has alternative mnemonic LD A,(HLI) or LDI A,(HL)
            LD (HL+),A has alternative mnemonic LD (HLI),A or LDI (HL),A
            LD A,(HL-) has alternative mnemonic LD A,(HLD) or LDD A,(HL)
            LD (HL-),A has alternative mnemonic LD (HLD),A or LDD (HL),A
            LD HL,SP+r8 has alternative mnemonic LDHL SP,r8

         Instruction STOP has according to manuals opcode 10 00 and thus is 2 bytes
         long. Anyhow it seems there is no reason for it so some assemblers code it
         simply as one byte instruction 10.

         Flags affected are always shown in Z H N C order. If flag is marked by "0" it
         means it is reset after the instruction. If it is marked by "1" it is set. If
         it is marked by "-" it is not changed. If it is marked by "Z", "N", "H" or "C"
         corresponding flag is affected as expected by its function.

         Duration of conditional calls and returns is different when action is taken
         or not. The cycles in the struct are the lower number, when action is
         _not_ taken. The higher number is in the comment on the right.
         */

        /* 0x00 - - - - */ { 1,  4, "NOP" },
        /* 0x01 - - - - */ { 3, 12, "LD BC,d16" },
        /* 0x02 - - - - */ { 1,  8, "LD (BC),A" },
        /* 0x03 - - - - */ { 1,  8, "INC BC" },
        /* 0x04 Z 0 H - */ { 1,  4, "INC B" },
        /* 0x05 Z 1 H - */ { 1,  4, "DEC B" },
        /* 0x06 - - - - */ { 2,  8, "LD B,d8" },
        /* 0x07 0 0 0 C */ { 1,  4, "RLCA" },
        /* 0x08 - - - - */ { 3, 20, "LD (a16),SP" },
        /* 0x09 - 0 H C */ { 1,  8, "ADD HL,BC" },
        /* 0x0A - - - - */ { 1,  8, "LD A,(BC)" },
        /* 0x0B - - - - */ { 1,  8, "DEC BC" },
        /* 0x0C Z 0 H - */ { 1,  4, "INC C" },
        /* 0x0D Z 1 H - */ { 1,  4, "DEC C" },
        /* 0x0E - - - - */ { 2,  8, "LD C,d8" },
        /* 0x0F 0 0 0 C */ { 1,  4, "RRCA" },
        /* 0x10 - - - - */ { 2,  4, "STOP 0" },
        /* 0x11 - - - - */ { 3, 12, "LD DE,d16" },
        /* 0x12 - - - - */ { 1,  8, "LD (DE),A" },
        /* 0x13 - - - - */ { 1,  8, "INC DE" },
        /* 0x14 Z 0 H - */ { 1,  4, "INC D" },
        /* 0x15 Z 1 H - */ { 1,  4, "DEC D" },
        /* 0x16 - - - - */ { 2,  8, "LD D,d8" },
        /* 0x17 0 0 0 C */ { 1,  4, "RLA" },
        /* 0x18 - - - - */ { 2, 12, "JR r8" },
        /* 0x19 - 0 H C */ { 1,  8, "ADD HL,DE" },
        /* 0x1A - - - - */ { 1,  8, "LD A,(DE)" },
        /* 0x1B - - - - */ { 1,  8, "DEC DE" },
        /* 0x1C Z 0 H - */ { 1,  4, "INC E" },
        /* 0x1D Z 1 H - */ { 1,  4, "DEC E" },
        /* 0x1E - - - - */ { 2,  8, "LD E,d8" },
        /* 0x1F 0 0 0 C */ { 1,  4, "RRA" },
        /* 0x20 - - - - */ { 2,  8, "JR NZ,r8" }, // (12 cycles when action taken)
        /* 0x21 - - - - */ { 3, 12, "LD HL,d16" },
        /* 0x22 - - - - */ { 1,  8, "LD (HL+),A" },
        /* 0x23 - - - - */ { 1,  8, "INC HL" },
        /* 0x24 Z 0 H - */ { 1,  4, "INC H" },
        /* 0x25 Z 1 H - */ { 1,  4, "DEC H" },
        /* 0x26 - - - - */ { 2,  8, "LD H,d8" },
        /* 0x27 Z - 0 C */ { 1,  4, "DAA" },
        /* 0x28 - - - - */ { 2,  8, "JR Z,r8" }, // (12 cycles when action taken)
        /* 0x29 - 0 H C */ { 1,  8, "ADD HL,HL" },
        /* 0x2A - - - - */ { 1,  8, "LD A,(HL+)" },
        /* 0x2B - - - - */ { 1,  8, "DEC HL" },
        /* 0x2C Z 0 H - */ { 1,  4, "INC L" },
        /* 0x2D Z 1 H - */ { 1,  4, "DEC L" },
        /* 0x2E - - - - */ { 2,  8, "LD L,d8" },
        /* 0x2F - 1 1 - */ { 1,  4, "CPL" },
        /* 0x30 - - - - */ { 2,  8, "JR NC,r8" }, // (12 cycles when action taken)
        /* 0x31 - - - - */ { 3, 12, "LD SP,d16" },
        /* 0x32 - - - - */ { 1,  8, "LD (HL-),A" },
        /* 0x33 - - - - */ { 1,  8, "INC SP" },
        /* 0x34 Z 0 H - */ { 1, 12, "INC (HL)" },
        /* 0x35 Z 1 H - */ { 1, 12, "DEC (HL)" },
        /* 0x36 - - - - */ { 2, 12, "LD (HL),d8" },
        /* 0x37 - 0 0 1 */ { 1,  4, "SCF" },
        /* 0x38 - - - - */ { 2,  8, "JR C,r8" }, // (12 cycles when action taken)
        /* 0x39 - 0 H C */ { 1,  8, "ADD HL,SP" },
        /* 0x3A - - - - */ { 1,  8, "LD A,(HL-)" },
        /* 0x3B - - - - */ { 1,  8, "DEC SP" },
        /* 0x3C Z 0 H - */ { 1,  4, "INC A" },
        /* 0x3D Z 1 H - */ { 1,  4, "DEC A" },
        /* 0x3E - - - - */ { 2,  8, "LD A,d8" },
        /* 0x3F - 0 0 C */ { 1,  4, "CCF" },
        /* 0x40 - - - - */ { 1,  4, "LD B,B" },
        /* 0x41 - - - - */ { 1,  4, "LD B,C" },
        /* 0x42 - - - - */ { 1,  4, "LD B,D" },
        /* 0x43 - - - - */ { 1,  4, "LD B,E" },
        /* 0x44 - - - - */ { 1,  4, "LD B,H" },
        /* 0x45 - - - - */ { 1,  4, "LD B,L" },
        /* 0x46 - - - - */ { 1,  8, "LD B,(HL)" },
        /* 0x47 - - - - */ { 1,  4, "LD B,A" },
        /* 0x48 - - - - */ { 1,  4, "LD C,B" },
        /* 0x49 - - - - */ { 1,  4, "LD C,C" },
        /* 0x4A - - - - */ { 1,  4, "LD C,D" },
        /* 0x4B - - - - */ { 1,  4, "LD C,E" },
        /* 0x4C - - - - */ { 1,  4, "LD C,H" },
        /* 0x4D - - - - */ { 1,  4, "LD C,L" },
        /* 0x4E - - - - */ { 1,  8, "LD C,(HL)" },
        /* 0x4F - - - - */ { 1,  4, "LD C,A" },
        /* 0x50 - - - - */ { 1,  4, "LD D,B" },
        /* 0x51 - - - - */ { 1,  4, "LD D,C" },
        /* 0x52 - - - - */ { 1,  4, "LD D,D" },
        /* 0x53 - - - - */ { 1,  4, "LD D,E" },
        /* 0x54 - - - - */ { 1,  4, "LD D,H" },
        /* 0x55 - - - - */ { 1,  4, "LD D,L" },
        /* 0x56 - - - - */ { 1,  8, "LD D,(HL)" },
        /* 0x57 - - - - */ { 1,  4, "LD D,A" },
        /* 0x58 - - - - */ { 1,  4, "LD E,B" },
        /* 0x59 - - - - */ { 1,  4, "LD E,C" },
        /* 0x5A - - - - */ { 1,  4, "LD E,D" },
        /* 0x5B - - - - */ { 1,  4, "LD E,E" },
        /* 0x5C - - - - */ { 1,  4, "LD E,H" },
        /* 0x5D - - - - */ { 1,  4, "LD E,L" },
        /* 0x5E - - - - */ { 1,  8, "LD E,(HL)" },
        /* 0x5F - - - - */ { 1,  4, "LD E,A" },
        /* 0x60 - - - - */ { 1,  4, "LD H,B" },
        /* 0x61 - - - - */ { 1,  4, "LD H,C" },
        /* 0x62 - - - - */ { 1,  4, "LD H,D" },
        /* 0x63 - - - - */ { 1,  4, "LD H,E" },
        /* 0x64 - - - - */ { 1,  4, "LD H,H" },
        /* 0x65 - - - - */ { 1,  4, "LD H,L" },
        /* 0x66 - - - - */ { 1,  8, "LD H,(HL)" },
        /* 0x67 - - - - */ { 1,  4, "LD H,A" },
        /* 0x68 - - - - */ { 1,  4, "LD L,B" },
        /* 0x69 - - - - */ { 1,  4, "LD L,C" },
        /* 0x6A - - - - */ { 1,  4, "LD L,D" },
        /* 0x6B - - - - */ { 1,  4, "LD L,E" },
        /* 0x6C - - - - */ { 1,  4, "LD L,H" },
        /* 0x6D - - - - */ { 1,  4, "LD L,L" },
        /* 0x6E - - - - */ { 1,  8, "LD L,(HL)" },
        /* 0x6F - - - - */ { 1,  4, "LD L,A" },
        /* 0x70 - - - - */ { 1,  8, "LD (HL),B" },
        /* 0x71 - - - - */ { 1,  8, "LD (HL),C" },
        /* 0x72 - - - - */ { 1,  8, "LD (HL),D" },
        /* 0x73 - - - - */ { 1,  8, "LD (HL),E" },
        /* 0x74 - - - - */ { 1,  8, "LD (HL),H" },
        /* 0x75 - - - - */ { 1,  8, "LD (HL),L" },
        /* 0x76 - - - - */ { 1,  4, "HALT" },
        /* 0x77 - - - - */ { 1,  8, "LD (HL),A" },
        /* 0x78 - - - - */ { 1,  4, "LD A,B" },
        /* 0x79 - - - - */ { 1,  4, "LD A,C" },
        /* 0x7A - - - - */ { 1,  4, "LD A,D" },
        /* 0x7B - - - - */ { 1,  4, "LD A,E" },
        /* 0x7C - - - - */ { 1,  4, "LD A,H" },
        /* 0x7D - - - - */ { 1,  4, "LD A,L" },
        /* 0x7E - - - - */ { 1,  8, "LD A,(HL)" },
        /* 0x7F - - - - */ { 1,  4, "LD A,A" },
        /* 0x80 Z 0 H C */ { 1,  4, "ADD A,B" },
        /* 0x81 Z 0 H C */ { 1,  4, "ADD A,C" },
        /* 0x82 Z 0 H C */ { 1,  4, "ADD A,D" },
        /* 0x83 Z 0 H C */ { 1,  4, "ADD A,E" },
        /* 0x84 Z 0 H C */ { 1,  4, "ADD A,H" },
        /* 0x85 Z 0 H C */ { 1,  4, "ADD A,L" },
        /* 0x86 Z 0 H C */ { 1,  8, "ADD A,(HL)" },
        /* 0x87 Z 0 H C */ { 1,  4, "ADD A,A" },
        /* 0x88 Z 0 H C */ { 1,  4, "ADC A,B" },
        /* 0x89 Z 0 H C */ { 1,  4, "ADC A,C" },
        /* 0x8A Z 0 H C */ { 1,  4, "ADC A,D" },
        /* 0x8B Z 0 H C */ { 1,  4, "ADC A,E" },
        /* 0x8C Z 0 H C */ { 1,  4, "ADC A,H" },
        /* 0x8D Z 0 H C */ { 1,  4, "ADC A,L" },
        /* 0x8E Z 0 H C */ { 1,  8, "ADC A,(HL)" },
        /* 0x8F Z 0 H C */ { 1,  4, "ADC A,A" },
        /* 0x90 Z 1 H C */ { 1,  4, "SUB B" },
        /* 0x91 Z 1 H C */ { 1,  4, "SUB C" },
        /* 0x92 Z 1 H C */ { 1,  4, "SUB D" },
        /* 0x93 Z 1 H C */ { 1,  4, "SUB E" },
        /* 0x94 Z 1 H C */ { 1,  4, "SUB H" },
        /* 0x95 Z 1 H C */ { 1,  4, "SUB L" },
        /* 0x96 Z 1 H C */ { 1,  8, "SUB (HL)" },
        /* 0x97 Z 1 H C */ { 1,  4, "SUB A" },
        /* 0x98 Z 1 H C */ { 1,  4, "SBC A,B" },
        /* 0x99 Z 1 H C */ { 1,  4, "SBC A,C" },
        /* 0x9A Z 1 H C */ { 1,  4, "SBC A,D" },
        /* 0x9B Z 1 H C */ { 1,  4, "SBC A,E" },
        /* 0x9C Z 1 H C */ { 1,  4, "SBC A,H" },
        /* 0x9D Z 1 H C */ { 1,  4, "SBC A,L" },
        /* 0x9E Z 1 H C */ { 1,  8, "SBC A,(HL)" },
        /* 0x9F Z 1 H C */ { 1,  4, "SBC A,A" },
        /* 0xA0 Z 0 1 0 */ { 1,  4, "AND B" },
        /* 0xA1 Z 0 1 0 */ { 1,  4, "AND C" },
        /* 0xA2 Z 0 1 0 */ { 1,  4, "AND D" },
        /* 0xA3 Z 0 1 0 */ { 1,  4, "AND E" },
        /* 0xA4 Z 0 1 0 */ { 1,  4, "AND H" },
        /* 0xA5 Z 0 1 0 */ { 1,  4, "AND L" },
        /* 0xA6 Z 0 1 0 */ { 1,  8, "AND (HL)" },
        /* 0xA7 Z 0 1 0 */ { 1,  4, "AND A" },
        /* 0xA8 Z 0 0 0 */ { 1,  4, "XOR B" },
        /* 0xA9 Z 0 0 0 */ { 1,  4, "XOR C" },
        /* 0xAA Z 0 0 0 */ { 1,  4, "XOR D" },
        /* 0xAB Z 0 0 0 */ { 1,  4, "XOR E" },
        /* 0xAC Z 0 0 0 */ { 1,  4, "XOR H" },
        /* 0xAD Z 0 0 0 */ { 1,  4, "XOR L" },
        /* 0xAE Z 0 0 0 */ { 1,  8, "XOR (HL)" },
        /* 0xAF Z 0 0 0 */ { 1,  4, "XOR A" },
        /* 0xB0 Z 0 0 0 */ { 1,  4, "OR B" },
        /* 0xB1 Z 0 0 0 */ { 1,  4, "OR C" },
        /* 0xB2 Z 0 0 0 */ { 1,  4, "OR D" },
        /* 0xB3 Z 0 0 0 */ { 1,  4, "OR E" },
        /* 0xB4 Z 0 0 0 */ { 1,  4, "OR H" },
        /* 0xB5 Z 0 0 0 */ { 1,  4, "OR L" },
        /* 0xB6 Z 0 0 0 */ { 1,  8, "OR (HL)" },
        /* 0xB7 Z 0 0 0 */ { 1,  4, "OR A" },
        /* 0xB8 Z 1 H C */ { 1,  4, "CP B" },
        /* 0xB9 Z 1 H C */ { 1,  4, "CP C" },
        /* 0xBA Z 1 H C */ { 1,  4, "CP D" },
        /* 0xBB Z 1 H C */ { 1,  4, "CP E" },
        /* 0xBC Z 1 H C */ { 1,  4, "CP H" },
        /* 0xBD Z 1 H C */ { 1,  4, "CP L" },
        /* 0xBE Z 1 H C */ { 1,  8, "CP (HL)" },
        /* 0xBF Z 1 H C */ { 1,  4, "CP A" },
        /* 0xC0 - - - - */ { 1,  8, "RET NZ" }, // (20 cycles when action taken)
        /* 0xC1 - - - - */ { 1, 12, "POP BC" },
        /* 0xC2 - - - - */ { 3, 12, "JP NZ,a16" }, // (16 cycles when action taken)
        /* 0xC3 - - - - */ { 3, 16, "JP a16" },
        /* 0xC4 - - - - */ { 3, 12, "CALL NZ,a16" }, // (24 cycles when action taken)
        /* 0xC5 - - - - */ { 1, 16, "PUSH BC" },
        /* 0xC6 Z 0 H C */ { 2,  8, "ADD A,d8" },
        /* 0xC7 - - - - */ { 1, 16, "RST 00H" },
        /* 0xC8 - - - - */ { 1,  8, "RET Z" }, // (20 cycles when action taken)
        /* 0xC9 - - - - */ { 1, 16, "RET" },
        /* 0xCA - - - - */ { 3, 12, "JP Z,a16" }, // (16 cycles when action taken)
        /* 0xCB - - - - */ { 1,  4, "PREFIX CB" },
        /* 0xCC - - - - */ { 3, 12, "CALL Z,a16" }, // (24 cycles when action taken)
        /* 0xCD - - - - */ { 3, 24, "CALL a16" },
        /* 0xCE Z 0 H C */ { 2,  8, "ADC A,d8" },
        /* 0xCF - - - - */ { 1, 16, "RST 08H" },
        /* 0xD0 - - - - */ { 1,  8, "RET NC" }, // (20 cycles when action taken)
        /* 0xD1 - - - - */ { 1, 12, "POP DE" },
        /* 0xD2 - - - - */ { 3, 12, "JP NC,a16" }, // (16 cycles when action taken)
        /* 0xD3         */ { 1,  0, "INVALID_INSTRUCTION" },
        /* 0xD4 - - - - */ { 3, 12, "CALL NC,a16" }, // (24 cycles when action taken)
        /* 0xD5 - - - - */ { 1, 16, "PUSH DE" },
        /* 0xD6 Z 1 H C */ { 2,  8, "SUB d8" },
        /* 0xD7 - - - - */ { 1, 16, "RST 10H" },
        /* 0xD8 - - - - */ { 1,  8, "RET C" }, // (20 cycles when action taken)
        /* 0xD9 - - - - */ { 1, 16, "RETI" },
        /* 0xDA - - - - */ { 3, 12, "JP C,a16" }, // (16 cycles when action taken)
        /* 0xDB         */ { 1,  0, "INVALID_INSTRUCTION" },
        /* 0xDC - - - - */ { 3, 12, "CALL C,a16" }, // (24 cycles when action taken)
        /* 0xDD         */ { 1,  0, "INVALID_INSTRUCTION" },
        /* 0xDE Z 1 H C */ { 2,  8, "SBC A,d8" },
        /* 0xDF - - - - */ { 1, 16, "RST 18H" },
        /* 0xE0 - - - - */ { 2, 12, "LDH (a8),A" },
        /* 0xE1 - - - - */ { 1, 12, "POP HL" },
        /* 0xE2 - - - - */ { 1,  8, "LD (C),A" },
        /* 0xE3         */ { 1,  0, "INVALID_INSTRUCTION" },
        /* 0xE4         */ { 1,  0, "INVALID_INSTRUCTION" },
        /* 0xE5 - - - - */ { 1, 16, "PUSH HL" },
        /* 0xE6 Z 0 1 0 */ { 2,  8, "AND d8" },
        /* 0xE7 - - - - */ { 1, 16, "RST 20H" },
        /* 0xE8 0 0 H C */ { 2, 16, "ADD SP,r8" },
        /* 0xE9 - - - - */ { 1,  4, "JP (HL)" },
        /* 0xEA - - - - */ { 3, 16, "LD (a16),A" },
        /* 0xEB         */ { 1,  0, "INVALID_INSTRUCTION" },
        /* 0xEC         */ { 1,  0, "INVALID_INSTRUCTION" },
        /* 0xED         */ { 1,  0, "INVALID_INSTRUCTION" },
        /* 0xEE Z 0 0 0 */ { 2,  8, "XOR d8" },
        /* 0xEF - - - - */ { 1, 16, "RST 28H" },
        /* 0xF0 - - - - */ { 2, 12, "LDH A,(a8)" },
        /* 0xF1 Z N H C */ { 1, 12, "POP AF" },
        /* 0xF2 - - - - */ { 1,  8, "LD A,(C)" },
        /* 0xF3 - - - - */ { 1,  4, "DI" },
        /* 0xF4         */ { 1,  0, "INVALID_INSTRUCTION" },
        /* 0xF5 - - - - */ { 1, 16, "PUSH AF" },
        /* 0xF6 Z 0 0 0 */ { 2,  8, "OR d8" },
        /* 0xF7 - - - - */ { 1, 16, "RST 30H" },
        /* 0xF8 0 0 H C */ { 2, 12, "LD HL,SP+r8" },
        /* 0xF9 - - - - */ { 1,  8, "LD SP,HL" },
        /* 0xFA - - - - */ { 3, 16, "LD A,(a16)" },
        /* 0xFB - - - - */ { 1,  4, "EI" },
        /* 0xFC         */ { 1,  0, "INVALID_INSTRUCTION" },
        /* 0xFD         */ { 1,  0, "INVALID_INSTRUCTION" },
        /* 0xFE Z 1 H C */ { 2,  8, "CP d8" },
        /* 0xFF - - - - */ { 1, 16, "RST 38H" }
    };
} // namespace CPU
