//
//  main.cpp
//  gemuboi
//
//  Created by Tom on 17/09/2015.
//
//
#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <cstring>

typedef unsigned char U8;
typedef unsigned short U16;

U8 NINTENDO_LOGO[48] = {
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
    0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
    0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
    0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

const size_t ROM_MAX_SIZE = 8 * 1024 * 1024; //8MiB
const size_t ROM_TITLE_SIZE = 17;

struct RomHeader {
    // 0000-0099: ???
    U8 dunno[256];

    // 0100-0103: cpu jumps to here when game starts
    U8 entry_point[4];

    // 0104-0133: must be equal to `NINTENDO_LOGO`
    U8 nintendo_logo[48];

    // 0134-013E: game title. might spill into the next two elements (up to 16
    // chars)
    U8 game_title[11];

    // 013F-0142: 4 byte manufacturer identifier
    U8 manufaturer_code[4];

    // 0144: upper bit enables CGB functions. Typical values are:
    //       80h - Game supports CGB functions, but works on old gameboys also.
    //       C0h - Game works on CGB only (physically the same as 80h).
    U8 cgb_flag;

    /* 0144-0145
       indicating the company or publisher of the game.
       These two bytes are used in newer games only.
       Older games are using the header entry at 014B instead.
     */
    U8 new_licensee_code[2];

    /* 0146
       Specifies whether the game supports SGB functions, common values are:
       00h = No SGB functions (Normal Gameboy or CGB only game)
       03h = Game supports SGB functions
     */
    U8 sgb_flag;

    /* 0147
       Specifies which Memory Bank Controller (if any) is used in the
       cartridge, and if further external hardware exists in the cartridge.

        00h  ROM ONLY                 13h  MBC3+RAM+BATTERY
        01h  MBC1                     15h  MBC4
        02h  MBC1+RAM                 16h  MBC4+RAM
        03h  MBC1+RAM+BATTERY         17h  MBC4+RAM+BATTERY
        05h  MBC2                     19h  MBC5
        06h  MBC2+BATTERY             1Ah  MBC5+RAM
        08h  ROM+RAM                  1Bh  MBC5+RAM+BATTERY
        09h  ROM+RAM+BATTERY          1Ch  MBC5+RUMBLE
        0Bh  MMM01                    1Dh  MBC5+RUMBLE+RAM
        0Ch  MMM01+RAM                1Eh  MBC5+RUMBLE+RAM+BATTERY
        0Dh  MMM01+RAM+BATTERY        FCh  POCKET CAMERA
        0Fh  MBC3+TIMER+BATTERY       FDh  BANDAI TAMA5
        10h  MBC3+TIMER+RAM+BATTERY   FEh  HuC3
        11h  MBC3                     FFh  HuC1+RAM+BATTERY
        12h  MBC3+RAM
     */
    U8 cartridge_type;

    /* 0148
     * Specifies the ROM Size of the cartridge. Typically calculated as "32KB shl N".
     *
     *   00h -  32KByte (no ROM banking)
     *   01h -  64KByte (4 banks)
     *   02h - 128KByte (8 banks)
     *   03h - 256KByte (16 banks)
     *   04h - 512KByte (32 banks)
     *   05h -   1MByte (64 banks)  - only 63 banks used by MBC1
     *   06h -   2MByte (128 banks) - only 125 banks used by MBC1
     *   07h -   4MByte (256 banks)
     *   52h - 1.1MByte (72 banks)
     *   53h - 1.2MByte (80 banks)
     *   54h - 1.5MByte (96 banks)
     */
    U8 rom_size;

    /* 0149
     * Specifies the size of the external RAM in the cartridge (if any).
     *   00h - None
     *   01h - 2 KBytes
     *   02h - 8 Kbytes
     *   03h - 32 KBytes (4 banks of 8KBytes each)
     *   04h - 128 KBytes (16 banks of 8KBytes each)
     *   05h - 64 KBytes (8 banks of 8KBytes each)
     */
    U8 ram_size;

    /* 014A - Destination Code
     * Specifies if this version of the game is supposed to be sold in japan,
     * or anywhere else. Only two values are defined.
     *  00h - Japanese
     *  01h - Non-Japanese
     */
    U8 destination_code;

    /* 014B - Old Licensee Code
     * Specifies the games company/publisher code in range 00-FFh. A value of
     * 33h signalizes that the New License Code in header bytes 0144-0145 is
     * used instead. (Super GameBoy functions won't work if <> $33.)
     */
    U8 old_licensee_code;

    /* 014C - Mask ROM Version number
     * Specifies the version number of the game. That is usually 00h.
     */
    U8 version_number;

    /* 014D - Header Checksum
     * Contains an 8 bit checksum across the cartridge header bytes 0134-014C. The checksum is calculated as follows:
     *  x=0:FOR i=0134h TO 014Ch:x=x-MEM[i]-1:NEXT
     * The lower 8 bits of the result must be the same than the value in this entry. The GAME WON'T WORK if this checksum is incorrect.
     */
    U8 header_checksum;

    /* 014E-014F - Global Checksum
     * Contains a 16 bit checksum (upper byte first) across the whole cartridge
     * ROM. Produced by adding all bytes of the cartridge (except for the two
     * checksum bytes). The Gameboy doesn't verify this checksum.
     */
    U8 global_checksum[2];
};

union Rom {
    RomHeader header;
    U8 data[ROM_MAX_SIZE];
};

struct MachineCodeDef {
    U8 byte_length;
    U8 cycles;
    const char* description;
};

const MachineCodeDef OPCODES[] = {
    /*
     Flag register (F) bits:

        7	6	5	4	3	2	1	0
        Z	N	H	C	0	0	0	0

        Z - Zero Flag
        N - Subtract Flag
        H - Half Carry Flag
        C - Carry Flag
        0 - Not used, always zero

     Direct value meanings:

        d8  means immediate 8 bit data
        d16 means immediate 16 bit data
        a8  means 8 bit unsigned data, which are added to $FF00 in certain instructions (replacement for missing IN and OUT instructions)
        a16 means 16 bit address
        r8  means 8 bit signed data, which are added to program counter

     Pseudo ASM meanings:

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
    /* 0xE2 - - - - */ { 2,  8, "LD (C),A" },
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
    /* 0xF2 - - - - */ { 2,  8, "LD A,(C)" },
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

// flag register bit masks
const U8 RFLAG_ZERO = 0x80;
const U8 RFLAG_SUBTRACT = 0x40;
const U8 RFLAG_HALFCARRY = 0x20;
const U8 RFLAG_CARRY = 0x10;

struct Registers {
    union {
        U16 af; // accumulator/flags
        struct { U8 a; U8 f; };
    };
    union {
        U16 bc;
        struct { U8 b; U8 c; };
    };
    union {
        U16 de;
        struct { U8 d; U8 e; };
    };
    union {
        U16 hl;
        struct { U8 h; U8 l; };
    };
    U16 sp; // stack pointer
    U16 pc; // program counter
};

struct Emulator {
    Registers registers;
    Rom rom;
    U8 memory[0xFFFF];

    /*
    Interrupt Enable Register
    --------------------------- FFFF
    Internal RAM
    --------------------------- FF80
    Empty but unusable for I/O
    --------------------------- FF4C
    I/O ports
    --------------------------- FF00
    Empty but unusable for I/O
    --------------------------- FEA0
    Sprite Attrib Memory (OAM)
    --------------------------- FE00
    Echo of 8kB Internal RAM
    --------------------------- E000
    8kB Internal RAM
    --------------------------- C000
    8kB switchable RAM bank
    --------------------------- A000
    8kB Video RAM
    ---------------------------￼ 8000 --
    16kB switchable ROM bank          |
    --------------------------- 4000  |= 32kB Cartrigbe
    16kB ROM bank #0                  |
    --------------------------- 0000 --
    */
};

void emu_init(Emulator* emu) {
    emu->registers.pc = 0x100; // rom entry point
    emu->registers.sp = 0xFFFE;
}

void* emu_address(Emulator* emu, U16 address) {
    /*
     $FFFF	        Interrupt Enable Flag
     $FF80-$FFFE	Zero Page - 127 bytes
     $FF00-$FF7F	Hardware I/O Registers
     $FEA0-$FEFF	Unusable Memory
     $FE00-$FE9F	OAM - Object Attribute Memory
     $E000-$FDFF	Echo RAM - Reserved, Do Not Use
     $D000-$DFFF	Internal RAM - Bank 1-7 (switchable - CGB only)
     $C000-$CFFF	Internal RAM - Bank 0 (fixed)
     $A000-$BFFF	Cartridge RAM (If Available)
     $9C00-$9FFF	BG Map Data 2
     $9800-$9BFF	BG Map Data 1
     $8000-$97FF	Character RAM
     $4000-$7FFF	Cartridge ROM - Switchable Banks 1-xx
     $0150-$3FFF	Cartridge ROM - Bank 0 (fixed)
     $0100-$014F	Cartridge Header Area
     $0000-$00FF	Restart and Interrupt Vectors
     */
    if(address < 0x4000){
        //TODO: implement bank switching
        return &(emu->rom.data[address]);
    } else {
        //TODO: check/implement these addresses properly
        return &(emu->memory[address]);
    }
}

template<typename T>
T emu_mem_read(Emulator* emu, U16 address) {
    T* addr = (T*)emu_address(emu, address);
    return *addr;
}

template<typename T>
void emu_mem_write(Emulator* emu, U16 address, T value) {
    T* addr = (T*)emu_address(emu, address);
    *addr = value;
}

template <typename T>
T rotl(T value) {
    return (value << 1) | (value >> (sizeof(value)*8 - 1));
}

template <typename T>
T rotr(T value) {
    return (value >> 1) | (value << (sizeof(value)*8 - 1));
}

#define GB_FLAG_SET(FLAG_BYTE, FLAG_MASK, ON_OFF) \
    do { \
        if(ON_OFF){ \
            FLAG_BYTE |= (FLAG_MASK); \
        } else { \
            FLAG_BYTE &= ~(FLAG_MASK); \
        } \
    } while(0)

#define GB_FLAG(FLAG_BYTE, FLAG_MASK) \
    ((FLAG_BYTE & FLAG_MASK) == FLAG_MASK)

inline
unsigned add_will_carry(U8 left_operand, U8 right_operand) {
    U16 promoted = (U16)left_operand + (U16)right_operand;
    return (promoted > 0x00FF);
}

inline
unsigned sub_will_borrow(U8 left_operand, U8 right_operand) {
    return (left_operand < right_operand);
}

inline
unsigned add_will_halfcarry(U8 left_operand, U8 right_operand) {
    U8 half = (left_operand & 0x0F) + (right_operand & 0x0F);
    return (half > 0x0F);
}

inline
unsigned sub_will_halfborrow(U8 left_operand, U8 right_operand) {
    return ((left_operand & 0x0F) < (right_operand & 0x0F));
}

inline
void flag_set_zero(U8* flags, U8 result) {
    GB_FLAG_SET(*flags, RFLAG_ZERO, (result == 0));
}

// returns number of cycles used
U8 emu_apply_next_instruction(Emulator* emu) {
    //TODO: add this base address to certain jump instructions
    //const U16 U8_JUMP_ADDR_BASE = 0xFF00;
    
    Registers* r = &emu->registers;
    U8* instr = (U8*)emu_address(emu, r->pc);
    U8 additional_cycles = 0; //for conditional instructions
    U16 jump_to_addr = 0;
#   define DIRECT_U16 (*((U16*)(&instr[1])))
#   define DIRECT_U8 (instr[1])
#   define READ8_HL emu_mem_read<U8>(emu, r->hl)

//        printf("0x%0.2X\n", instr[0]);

    switch(instr[0]){

        case 0x00: // NOP (- - - -)
            break;

        case 0x01: // LD BC,d16 (- - - -)
            r->bc = DIRECT_U16;
            break;

        case 0x02:{// LD (BC),A (- - - -)
            emu_mem_write(emu, r->bc, r->a);
            break;}

        case 0x03: // INC BC (- - - -)
            r->bc += 1;
            break;

// increments a single 8bit register
// assumes (Z 0 H -)
#define INC_R_IMPL(REGISTER) \
    do { \
        GB_FLAG_SET(r->f, RFLAG_HALFCARRY, add_will_halfcarry((REGISTER), 1)); \
        REGISTER += 1; \
        GB_FLAG_SET(r->f, RFLAG_ZERO, ((REGISTER) == 0)); \
        GB_FLAG_SET(r->f, RFLAG_SUBTRACT, 0); \
    } while(0)

        case 0x04: // INC B (Z 0 H -)
            INC_R_IMPL(r->b);
            break;

// decrements a single 8bit register
// assumes (Z 1 H -)
#define DEC_R_IMPL(REGISTER) \
    do { \
        GB_FLAG_SET(r->f, RFLAG_HALFCARRY, sub_will_halfborrow((REGISTER), 1));\
        REGISTER -= 1; \
        GB_FLAG_SET(r->f, RFLAG_ZERO, ((REGISTER) == 0)); \
        GB_FLAG_SET(r->f, RFLAG_SUBTRACT, 1); \
    } while(0)

        case 0x05: // DEC B (Z 1 H -)
            DEC_R_IMPL(r->b);
            break;

        case 0x06: // LD B,d8 (- - - -)
            r->b = DIRECT_U8;
            break;

        case 0x07:{// RLCA (0 0 0 C)
            //TODO: resolve conlflicting specs on how to set zero flag
            /*
                Bitwise cycle left.
                Old high bit (7) becomes the new low bit (0).
                Old high bit (7) also gets stored in the carry flag.
              
                        +---------------------------------+
                        |                                 |
                        |                                 v
                +---+   |   +---+---+---+---+---+---+---+---+
                | C |<--+---| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
                +---+       +---+---+---+---+---+---+---+---+

             */
            U8 high_bit = (r->a & 0x80); //remember high bit
            r->a = (r->a << 1) | (high_bit >> 7);
            GB_FLAG_SET(r->f, RFLAG_ZERO, 0);
            GB_FLAG_SET(r->f, RFLAG_SUBTRACT, 0);
            GB_FLAG_SET(r->f, RFLAG_HALFCARRY, 0);
            GB_FLAG_SET(r->f, RFLAG_CARRY, high_bit); // high bit goes into carry
            break;}

        case 0x08: // LD (a16),SP (- - - -)
            emu_mem_write(emu, DIRECT_U16, r->sp);
            break;

        case 0x09: // ADD HL,BC (- 0 H C)
            GB_FLAG_SET(r->f, RFLAG_SUBTRACT, 0);
            GB_FLAG_SET(r->f, RFLAG_HALFCARRY, add_will_halfcarry(r->hl, r->bc));
            GB_FLAG_SET(r->f, RFLAG_CARRY, add_will_carry(r->hl, r->bc));
            r->hl += r->bc;
            break;

        case 0x0A: // LD A,(BC) (- - - -)
            r->a = emu_mem_read<U8>(emu, r->bc);
            break;

        case 0x0B: // DEC BC (- - - -)
            r->bc -= 1;
            break;

        case 0x0C: // INC C (Z 0 H -)
            INC_R_IMPL(r->c);
            break;

        case 0x0D: // DEC C (Z 1 H -)
            DEC_R_IMPL(r->c);
            break;

        case 0x0E: // LD C,d8 (- - - -)
            r->c = DIRECT_U8;
            break;

        case 0x0F:{// RRCA (0 0 0 C)
            //TODO: resolve conlflicting specs on how to set zero flag
            /*
                Bitwise rotate A right.
                Old low bit (0) becomes the new high bit (7).
                Old low bit (0) to the carry flag.

                  +-----+---------------------------------+
                  |     |                                 |
                  v     |                                 |
                +---+   |   +---+---+---+---+---+---+---+---+
                | C |   +-->| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
                +---+       +---+---+---+---+---+---+---+---+
             */
            U8 low_bit = (r->a & 0x01);
            r->a = (r->a >> 1) | (low_bit << 7);
            GB_FLAG_SET(r->f, RFLAG_ZERO, 0);
            GB_FLAG_SET(r->f, RFLAG_SUBTRACT, 0);
            GB_FLAG_SET(r->f, RFLAG_HALFCARRY, 0);
            GB_FLAG_SET(r->f, RFLAG_CARRY, low_bit);
            break;}

        case 0x10: // STOP 0 (- - - -)
            //TODO: halt CPU and LCD display until button is pressed
            break;

        case 0x11: // LD DE,d16 (- - - -)
            r->de = DIRECT_U16;
            break;

        case 0x12: // LD (DE),A (- - - -)
            emu_mem_write(emu, r->de, r->a);
            break;

        case 0x13: // INC DE (- - - -)
            r->de += 1;
            break;

        case 0x14: // INC D (Z 0 H -)
            INC_R_IMPL(r->d);
            break;

        case 0x15: // DEC D (Z 1 H -)
            DEC_R_IMPL(r->d);
            break;

        case 0x16: // LD D,d8 (- - - -)
            r->d = DIRECT_U8;
            break;

        case 0x17:{// RLA (0 0 0 C)
            //TODO: resolve conlflicting specs on how to set zero flag
            /*
                Bitwise rotate A left through carry flag.
                Old high bit (7) becomes the new carry flag.
                Old carry flag becomes the new low bit (0).

                  +---------------------------------------+
                  |                                       |
                  |                                       v
                +---+       +---+---+---+---+---+---+---+---+
                | C |<------| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
                +---+       +---+---+---+---+---+---+---+---+
             */
            U8 old_carry_low_bit = (GB_FLAG(r->f, RFLAG_CARRY) ? 0x01 : 0x00);
            U8 old_high_bit = (r->a & 0x80);
            r->a = (r->a << 1) | old_carry_low_bit;
            GB_FLAG_SET(r->f, RFLAG_ZERO, 0);
            GB_FLAG_SET(r->f, RFLAG_SUBTRACT, 0);
            GB_FLAG_SET(r->f, RFLAG_HALFCARRY, 0);
            GB_FLAG_SET(r->f, RFLAG_CARRY, old_high_bit);
            break;}

        case 0x18: // JR r8 (- - - -)
            jump_to_addr = r->pc + (U16)DIRECT_U8;
            break;

        case 0x19: // ADD HL,DE (- 0 H C)
            GB_FLAG_SET(r->f, RFLAG_SUBTRACT, 0);
            GB_FLAG_SET(r->f, RFLAG_HALFCARRY, add_will_halfcarry(r->hl, r->de));
            GB_FLAG_SET(r->f, RFLAG_CARRY, add_will_carry(r->hl, r->de));
            r->hl += r->de;
            break;

        case 0x1A: // LD A,(DE) (- - - -)
            r->a = emu_mem_read<U8>(emu, r->de);
            break;

        case 0x1B: // DEC DE (- - - -)
            r->de -= 1;
            break;

        case 0x1C: // INC E (Z 0 H -)
            INC_R_IMPL(r->e);
            break;

        case 0x1D: // DEC E (Z 1 H -)
            DEC_R_IMPL(r->e);
            break;

        case 0x1E: // LD E,d8 (- - - -)
            r->e = DIRECT_U8;
            break;

        case 0x1F:{// RRA (0 0 0 C)
            //TODO: resolve conlflicting specs on how to set zero flag
            /*
                Bitwise rotate A right through carry flag.
                Old low bit (0) becomes the new carry flag.
                Old carry flag becomes the new high bit (7).

                  +---------------------------------------+
                  |                                       |
                  v                                       |
                +---+       +---+---+---+---+---+---+---+---+
                | C |------>| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
                +---+       +---+---+---+---+---+---+---+---+
             */
            U8 old_low_bit = (r->a & 0x01);
            U8 old_carry = (GB_FLAG(r->f, RFLAG_CARRY) ? 0x80 : 0x00);
            r->a = (r->a >> 1) | old_carry;
            GB_FLAG_SET(r->f, RFLAG_ZERO, 0);
            GB_FLAG_SET(r->f, RFLAG_SUBTRACT, 0);
            GB_FLAG_SET(r->f, RFLAG_HALFCARRY, 0);
            GB_FLAG_SET(r->f, RFLAG_CARRY, old_low_bit);
        break;}

        case 0x20: // JR NZ,r8 (- - - -)
            if(!GB_FLAG(r->f, RFLAG_ZERO)){
                additional_cycles = 4;
                jump_to_addr = r->pc + DIRECT_U8;
            }
            break;

        case 0x21: // LD HL,d16 (- - - -)
            r->hl = DIRECT_U16;
            break;

        case 0x22: // LD (HL+),A (- - - -)
            emu_mem_write(emu, r->hl, r->a);
            r->hl += 1;
            break;

        case 0x23: // INC HL (- - - -)
            r->hl += 1;
            break;

        case 0x24: // INC H (Z 0 H -)
            INC_R_IMPL(r->h);
            break;

        case 0x25: // DEC H (Z 1 H -)
            DEC_R_IMPL(r->e);
            break;

        case 0x26: // LD H,d8 (- - - -)
            r->h = DIRECT_U8;
            break;

        case 0x27:{// DAA (Z - 0 C)
            /*
             When this instruction is executed, the A register is BCD corrected using the contents
             of the flags. The exact process is the following: if the least significant four bits
             of A contain a non-BCD digit (i. e. it is greater than 9) or the H flag is set, then
             $06 is added to the register. Then the four most significant bits are checked. If this
             more significant digit also happens to be greater than 9 or the C flag is set, then
             $60 is added.
             
             The same rule applies when N=1. The only thing is that you have to subtract
             the correction when N=1.
             
             If the second addition was needed, the C flag is set after execution, 
             otherwise it is reset.
             */
            U8 subtract = GB_FLAG(r->f, RFLAG_SUBTRACT);

            if((r->a & 0x0F) > 0x09 || GB_FLAG(r->f, RFLAG_HALFCARRY)){
                if(subtract){
                    r->a -= 0x06;
                } else {
                    r->a += 0x06;
                }
            }

            if((r->a & 0xF0) > 0x90 || GB_FLAG(r->f, RFLAG_CARRY)){
                if(subtract){
                    r->a -= 0x60;
                } else {
                    r->a += 0x60;
                }
                GB_FLAG_SET(r->f, RFLAG_CARRY, 1);
            } else {
                GB_FLAG_SET(r->f, RFLAG_CARRY, 0);
            }

            GB_FLAG_SET(r->f, RFLAG_ZERO, (r->a == 0));
            GB_FLAG_SET(r->f, RFLAG_HALFCARRY, 0);
            break;}

        case 0x28: // JR Z,r8 (- - - -)
            if(GB_FLAG(r->f, RFLAG_ZERO)){
                additional_cycles = 4;
                jump_to_addr = r->pc + DIRECT_U8;
            }
            break;

        case 0x29: // ADD HL,HL (- 0 H C)
            //TODO: flags here
            r->hl += r->hl;
            break;

        case 0x2A: // LD A,(HL+)
            r->a = READ8_HL;
            r->hl += 1;
            break;
        case 0x2B: // DEC HL
            r->hl -= 1;
            break;
        case 0x2C: // INC L
            INC_R_IMPL(r->l);
            break;
        case 0x2D: // DEC L
            DEC_R_IMPL(r->l);
            break;
        case 0x2E: // LD L,d8
            r->l = DIRECT_U8;
            break;
        case 0x2F: // CPL
            r->a = ~(r->a);
            break;
        case 0x30: // JR NC,r8
            if(!GB_FLAG(r->f, RFLAG_CARRY)){
                additional_cycles = 4;
                r->pc += DIRECT_U8;
            }
            break;
        case 0x31: // LD SP,d16
            r->sp = DIRECT_U16;
            break;
        case 0x32: // LD (HL-),A
            emu_mem_write(emu, r->hl, r->a);
            r->hl -= 1;
            break;
        case 0x33: // INC SP
            r->sp += 1;
            break;
        case 0x34: // INC (HL)
            emu_mem_write(emu, r->hl, READ8_HL + 1);
            break;
        case 0x35: // DEC (HL)
            emu_mem_write(emu, r->hl, READ8_HL - 1);
            break;
        case 0x36: // LD (HL),d8
            emu_mem_write(emu, r->hl, DIRECT_U8);
            break;
        case 0x37: // SCF
            r->f |= RFLAG_CARRY;
            break;
        case 0x38: // JR C,r8
            if(GB_FLAG(r->f, RFLAG_CARRY)){
                additional_cycles = 4;
                r->pc += DIRECT_U8;
            }
            break;
        case 0x39: // ADD HL,SP
            r->hl += r->sp;
            break;
        case 0x3A: // LD A,(HL-)
            r->a = READ8_HL;
            r->hl -= 1;
            break;
        case 0x3B: // DEC SP
            r->sp -= 1;
            break;
        case 0x3C: // INC A
            INC_R_IMPL(r->a);
            break;
        case 0x3D: // DEC A
            DEC_R_IMPL(r->a);
            break;
        case 0x3E: // LD A,d8
            r->a = DIRECT_U8;
            break;
        case 0x3F: // CCF
            r->f ^= RFLAG_CARRY;
            break;
        case 0x40: // LD B,B
            // NOP
            break;
        case 0x41: // LD B,C
            r->b = r->c;
            break;
        case 0x42: // LD B,D
            r->b = r->d;
            break;
        case 0x43: // LD B,E
            r->b = r->e;
            break;
        case 0x44: // LD B,H
            r->b = r->h;
            break;
        case 0x45: // LD B,L
            r->b = r->l;
            break;
        case 0x46: // LD B,(HL)
            r->b = READ8_HL;
            break;
        case 0x47: // LD B,A
            r->b = r->a;
            break;
        case 0x48: // LD C,B
            r->c = r->b;
            break;
        case 0x49: // LD C,C
            // NOP
            break;
        case 0x4A: // LD C,D
            r->c = r->d;
            break;
        case 0x4B: // LD C,E
            r->c = r->e;
            break;
        case 0x4C: // LD C,H
            r->c = r->h;
            break;
        case 0x4D: // LD C,L
            r->c = r->l;
            break;
        case 0x4E: // LD C,(HL)
            r->c = READ8_HL;
            break;
        case 0x4F: // LD C,A
            r->c = r-> a;
            break;
        case 0x50: // LD D,B
            r->d = r->b;
            break;
        case 0x51: // LD D,C
            r->d = r->c;
            break;
        case 0x52: // LD D,D
            // NOP
            break;
        case 0x53: // LD D,E
            r->d = r->e;
            break;
        case 0x54: // LD D,H
            r->d = r->h;
            break;
        case 0x55: // LD D,L
            r->d = r->l;
            break;
        case 0x56: // LD D,(HL)
            r->d = READ8_HL;
            break;
        case 0x57: // LD D,A
            r->d = r->a;
            break;
        case 0x58: // LD E,B
            r->e = r->b;
            break;
        case 0x59: // LD E,C
            r->e = r->c;
            break;
        case 0x5A: // LD E,D
            r->e = r->d;
            break;
        case 0x5B: // LD E,E
            // NOP
            break;
        case 0x5C: // LD E,H
            r->e = r->h;
            break;
        case 0x5D: // LD E,L
            r->e = r->l;
            break;
        case 0x5E: // LD E,(HL)
            r->e = READ8_HL;
            break;
        case 0x5F: // LD E,A
            r->e = r->a;
            break;
        case 0x60: // LD H,B
            r->h = r->b;
            break;
        case 0x61: // LD H,C
            r->h = r->c;
            break;
        case 0x62: // LD H,D
            r->h = r->d;
            break;
        case 0x63: // LD H,E
            r->h = r->e;
            break;
        case 0x64: // LD H,H
            // NOP
            break;
        case 0x65: // LD H,L
            r->h = r->l;
            break;
        case 0x66: // LD H,(HL)
            r->h = READ8_HL;
            break;
        case 0x67: // LD H,A
            r->h = r->a;
            break;
        case 0x68: // LD L,B
            r->l = r->b;
            break;
        case 0x69: // LD L,C
            r->l = r->c;
            break;
        case 0x6A: // LD L,D
            r->l = r->d;
            break;
        case 0x6B: // LD L,E
            r->l = r->e;
            break;
        case 0x6C: // LD L,H
            r->l = r->h;
            break;
        case 0x6D: // LD L,L
            // NOP
            break;
        case 0x6E: // LD L,(HL)
            r->l = READ8_HL;
            break;
        case 0x6F: // LD L,A
            r->l = r->a;
            break;
        case 0x70: // LD (HL),B
            emu_mem_write(emu, r->hl, r->b);
            break;
        case 0x71: // LD (HL),C
            emu_mem_write(emu, r->hl, r->c);
            break;
        case 0x72: // LD (HL),D
            emu_mem_write(emu, r->hl, r->d);
            break;
        case 0x73: // LD (HL),E
            emu_mem_write(emu, r->hl, r->e);
            break;
        case 0x74: // LD (HL),H
            emu_mem_write(emu, r->hl, r->h);
            break;
        case 0x75: // LD (HL),L
            emu_mem_write(emu, r->hl, r->l);
            break;
        case 0x76: // HALT
            //TODO: here
            break;
        case 0x77: // LD (HL),A
            emu_mem_write(emu, r->hl, r->a);
            break;
        case 0x78: // LD A,B
            r->a = r->b;
            break;
        case 0x79: // LD A,C
            r->a = r->c;
            break;
        case 0x7A: // LD A,D
            r->a = r->d;
            break;
        case 0x7B: // LD A,E
            r->a = r->e;
            break;
        case 0x7C: // LD A,H
            r->a = r->h;
            break;
        case 0x7D: // LD A,L
            r->a = r->l;
            break;
        case 0x7E: // LD A,(HL)
            r->a = READ8_HL;
            break;
        case 0x7F: // LD A,A
            // NOP
            break;
        case 0x80: // ADD A,B
            r->a += r->b;
            break;
        case 0x81: // ADD A,C
            r->a += r->c;
            break;
        case 0x82: // ADD A,D
            r->a += r->d;
            break;
        case 0x83: // ADD A,E
            r->a += r->e;
            break;
        case 0x84: // ADD A,H
            r->a += r->h;
            break;
        case 0x85: // ADD A,L
            r->a += r->l;
            break;
        case 0x86: // ADD A,(HL)
            r->a += READ8_HL;
            break;
        case 0x87: // ADD A,A
            r->a += r->a;
            break;

#define ADC_A_IMPL(ADD_VALUE) \
    do { \
        r->a += ADD_VALUE; \
        if(GB_FLAG(r->f, RFLAG_CARRY)){ \
            r->a += 1; \
        } \
    } while(0)

        case 0x88: // ADC A,B
            ADC_A_IMPL(r->b);
            break;
        case 0x89: // ADC A,C
            ADC_A_IMPL(r->c);
            break;
        case 0x8A: // ADC A,D
            ADC_A_IMPL(r->d);
            break;
        case 0x8B: // ADC A,E
            ADC_A_IMPL(r->e);
            break;
        case 0x8C: // ADC A,H
            ADC_A_IMPL(r->h);
            break;
        case 0x8D: // ADC A,L
            ADC_A_IMPL(r->l);
            break;
        case 0x8E: // ADC A,(HL)
            ADC_A_IMPL(READ8_HL);
            break;
        case 0x8F: // ADC A,A
            ADC_A_IMPL(r->a);
            break;

#define SUB_A_IMPL(SUB_VALUE) \
    do { \
        r->a -= SUB_VALUE; \
    } while(0)

        case 0x90: // SUB B
            SUB_A_IMPL(r->b);
            break;
        case 0x91: // SUB C
            SUB_A_IMPL(r->c);
            break;
        case 0x92: // SUB D
            SUB_A_IMPL(r->d);
            break;
        case 0x93: // SUB E
            SUB_A_IMPL(r->e);
            break;
        case 0x94: // SUB H
            SUB_A_IMPL(r->h);
            break;
        case 0x95: // SUB L
            SUB_A_IMPL(r->l);
            break;
        case 0x96: // SUB (HL)
            SUB_A_IMPL(READ8_HL);
            break;
        case 0x97: // SUB A
            SUB_A_IMPL(r->a);
            break;

#define SBC_A_IMPL(SUB_VALUE) \
    do { \
        r->a -= SUB_VALUE; \
        if(GB_FLAG(r->f, RFLAG_CARRY)){ \
            r->a -= 1; \
        } \
    } while(0)

        case 0x98: // SBC A,B
            SBC_A_IMPL(r->b);
            break;
        case 0x99: // SBC A,C
            SBC_A_IMPL(r->c);
            break;
        case 0x9A: // SBC A,D
            SBC_A_IMPL(r->d);
            break;
        case 0x9B: // SBC A,E
            SBC_A_IMPL(r->e);
            break;
        case 0x9C: // SBC A,H
            SBC_A_IMPL(r->h);
            break;
        case 0x9D: // SBC A,L
            SBC_A_IMPL(r->l);
            break;
        case 0x9E: // SBC A,(HL)
            SBC_A_IMPL(READ8_HL);
            break;
        case 0x9F: // SBC A,A
            SBC_A_IMPL(r->a);
            break;

#define AND_A_IMPL(OTHER_VALUE) \
    do { \
        r->a = (r->a && OTHER_VALUE); \
    } while(0)

        case 0xA0: // AND B
            AND_A_IMPL(r->b);
            break;
        case 0xA1: // AND C
            AND_A_IMPL(r->c);
            break;
        case 0xA2: // AND D
            AND_A_IMPL(r->d);
            break;
        case 0xA3: // AND E
            AND_A_IMPL(r->e);
            break;
        case 0xA4: // AND H
            AND_A_IMPL(r->h);
            break;
        case 0xA5: // AND L
            AND_A_IMPL(r->l);
            break;
        case 0xA6: // AND (HL)
            AND_A_IMPL(READ8_HL);
            break;
        case 0xA7: // AND A
            AND_A_IMPL(r->a);
            break;

#define XOR_A_IMPL(OTHER_VALUE) \
    do { \
        r->a = (r->a ^ OTHER_VALUE); \
    } while(0)

        case 0xA8: // XOR B
            XOR_A_IMPL(r->b);
            break;
        case 0xA9: // XOR C
            XOR_A_IMPL(r->c);
            break;
        case 0xAA: // XOR D
            XOR_A_IMPL(r->d);
            break;
        case 0xAB: // XOR E
            XOR_A_IMPL(r->e);
            break;
        case 0xAC: // XOR H
            XOR_A_IMPL(r->h);
            break;
        case 0xAD: // XOR L
            XOR_A_IMPL(r->l);
            break;
        case 0xAE: // XOR (HL)
            XOR_A_IMPL(READ8_HL);
            break;
        case 0xAF: // XOR A
            XOR_A_IMPL(r->a);
            break;

#define OR_A_IMPL(OTHER_VALUE) \
    do { \
        r->a = (r->a || OTHER_VALUE); \
    } while(0)

        case 0xB0: // OR B
            OR_A_IMPL(r->b);
            break;
        case 0xB1: // OR C
            OR_A_IMPL(r->c);
            break;
        case 0xB2: // OR D
            OR_A_IMPL(r->d);
            break;
        case 0xB3: // OR E
            OR_A_IMPL(r->e);
            break;
        case 0xB4: // OR H
            OR_A_IMPL(r->h);
            break;
        case 0xB5: // OR L
            OR_A_IMPL(r->l);
            break;
        case 0xB6: // OR (HL)
            OR_A_IMPL(READ8_HL);
            break;
        case 0xB7: // OR A
            OR_A_IMPL(r->a);
            break;

//TODO: implement this
#define CP_A_IMPL(OTHER_VALUE) \
    do { \
    } while(0)

        case 0xB8: // CP B
            CP_A_IMPL(r->b);
            break;
        case 0xB9: // CP C
            CP_A_IMPL(r->c);
            break;
        case 0xBA: // CP D
            CP_A_IMPL(r->d);
            break;
        case 0xBB: // CP E
            CP_A_IMPL(r->e);
            break;
        case 0xBC: // CP H
            CP_A_IMPL(r->h);
            break;
        case 0xBD: // CP L
            CP_A_IMPL(r->l);
            break;
        case 0xBE: // CP (HL)
            CP_A_IMPL(READ8_HL);
            break;
        case 0xBF: // CP A
            CP_A_IMPL(r->a);
            break;

//TODO: implement this
#define RET_IMPL \
    do { \
    } while(0)

        case 0xC0: // RET NZ
            if(!GB_FLAG(r->f, RFLAG_ZERO)){
                RET_IMPL;
            }
            break;
        case 0xC1: // POP BC
            break;
        case 0xC2: // JP NZ,a16
            break;
        case 0xC3: // JP a16
            break;
        case 0xC4: // CALL NZ,a16
            break;
        case 0xC5: // PUSH BC
            break;
        case 0xC6: // ADD A,d8
            break;
        case 0xC7: // RST 00H
            break;
        case 0xC8: // RET Z
            if(GB_FLAG(r->f, RFLAG_ZERO)){
                RET_IMPL;
            }
            break;
        case 0xC9: // RET
            RET_IMPL;
            break;
        case 0xCA: // JP Z,a16
            break;
        case 0xCB: // PREFIX CB
            break;
        case 0xCC: // CALL Z,a16
            break;
        case 0xCD: // CALL a16
            break;
        case 0xCE: // ADC A,d8
            break;
        case 0xCF: // RST 08H
            break;
        case 0xD0: // RET NC
            if(!GB_FLAG(r->f, RFLAG_CARRY)){
                RET_IMPL;
            }
            break;
        case 0xD1: // POP DE
            break;
        case 0xD2: // JP NC,a16
            break;
        case 0xD3: // INVALID_INSTRUCTION
            break;
        case 0xD4: // CALL NC,a16
            break;
        case 0xD5: // PUSH DE
            break;
        case 0xD6: // SUB d8
            break;
        case 0xD7: // RST 10H
            break;
        case 0xD8: // RET C
            if(GB_FLAG(r->f, RFLAG_CARRY)){
                RET_IMPL;
            }
            break;
        case 0xD9: // RETI
            RET_IMPL;
            //TODO: enable interrupts here
            break;
        case 0xDA: // JP C,a16
            break;
        case 0xDB: // INVALID_INSTRUCTION
            break;
        case 0xDC: // CALL C,a16
            break;
        case 0xDD: // INVALID_INSTRUCTION
            break;
        case 0xDE: // SBC A,d8
            break;
        case 0xDF: // RST 18H
            break;
        case 0xE0: // LDH (a8),A
            break;
        case 0xE1: // POP HL
            break;
        case 0xE2: // LD (C),A
            break;
        case 0xE3: // INVALID_INSTRUCTION
            break;
        case 0xE4: // INVALID_INSTRUCTION
            break;
        case 0xE5: // PUSH HL
            break;
        case 0xE6: // AND d8
            break;
        case 0xE7: // RST 20H
            break;
        case 0xE8: // ADD SP,r8
            break;
        case 0xE9: // JP (HL)
            break;
        case 0xEA: // LD (a16),A
            break;
        case 0xEB: // INVALID_INSTRUCTION
            break;
        case 0xEC: // INVALID_INSTRUCTION
            break;
        case 0xED: // INVALID_INSTRUCTION
            break;
        case 0xEE: // XOR d8
            break;
        case 0xEF: // RST 28H
            break;
        case 0xF0: // LDH A,(a8)
            break;
        case 0xF1: // POP AF
            break;
        case 0xF2: // LD A,(C)
            break;
        case 0xF3: // DI
            break;
        case 0xF4: // INVALID_INSTRUCTION
            break;
        case 0xF5: // PUSH AF
            break;
        case 0xF6: // OR d8
            break;
        case 0xF7: // RST 30H
            break;
        case 0xF8: // LD HL,SP+r8
            break;
        case 0xF9: // LD SP,HL
            break;
        case 0xFA: // LD A,(a16)
            break;
        case 0xFB: // EI
            break;
        case 0xFC: // INVALID_INSTRUCTION
            break;
        case 0xFD: // INVALID_INSTRUCTION
            break;
        case 0xFE: // CP d8
            break;
        case 0xFF: // RST 38H
            break;
    }

    const MachineCodeDef* opcode = &OPCODES[*instr];
    if(jump_to_addr == 0){
        // step to next instruction
        r->pc += opcode->byte_length;
    } else {
        // jump to an arbitrary address
        r->pc = jump_to_addr;
    }
    return opcode->cycles + additional_cycles;

#   undef DIRECT_U16
#   undef DIRECT_U8
#   undef READ8_HL
}

void emu_step(Emulator* emu) {
    U8 cycles = emu_apply_next_instruction(emu);
    cycles += 1;
    //TODO: update total cycles elapsed in emulator
}

void rom_fread(Rom* rom, const char* filename) {
    FILE* f = fopen(filename, "rb");
    assert(f);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    assert(size < ROM_MAX_SIZE);
    rewind(f);

    fread(rom, size, 1, f);
    fclose(f);
}

void rom_get_title(Rom* rom, char* out_title) {
    strncpy(out_title, (const char*)&(rom->header.game_title), ROM_TITLE_SIZE - 1);
    out_title[ROM_TITLE_SIZE - 1] = 0;
}

int main(int argc, const char * argv[]) {
    assert(argc == 2);

    Emulator* emu = new Emulator;
    rom_fread(&emu->rom, argv[1]);

    char title[ROM_TITLE_SIZE] = {0};
    rom_get_title(&emu->rom, title);
    printf("Rom is %s\n", title);

    emu_init(emu);
//    emu_step(emu);
//    emu_step(emu);
}

