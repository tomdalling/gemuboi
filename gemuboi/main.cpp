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

#include "types.hpp"
#include "cart.hpp"
#include "cpu.hpp"


struct Emulator {
    CPU::Registers registers;
    Cart::Cart cart;

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
    U8 memory[0xFFFF];

    /*
     TODO:
     - cycles elapsed
     - stop instruction
     - halt instruction
     - interrupts enabled/disabled
     */
};

void emu_init(Emulator* emu) {
    memset(&emu->registers, 0, sizeof(emu->registers));
    emu->registers.pc = 0x100; // cart entry point
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
        return &(emu->cart.data[address]);
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
unsigned u16_add_will_halfcarry(U16 hl, U16 operand) {
    // for the `ADD HL,?` intruction, halfcarry occurs
    // for bit 11. 0x07FF is the mask for the lower 11 bits.
    U16 lower_11s_added = (hl & 0x07FF) + (operand & 0x07FF);
    return (lower_11s_added > 0x07FF);
}

inline
unsigned u16_add_will_carry(U16 hl, U16 operand) {
    U32 promoted = (U32)hl + (U32)operand;
    return (promoted > 0x0000FFFF);
}

U16 emu_stack_pop(Emulator* emu) {
    U16 retval = emu_mem_read<U16>(emu, emu->registers.sp);
    emu->registers.sp += 2;
    return retval;
}

void emu_stack_push(Emulator* emu, U16 value) {
    emu->registers.sp -= 2;
    emu_mem_write(emu, emu->registers.sp, value);
}

// returns number of cycles used
U8 emu_apply_next_instruction(Emulator* emu) {
    CPU::Registers* r = &emu->registers;
    U8* instr = (U8*)emu_address(emu, r->pc);
    const CPU::OpcodeDesc* opcode = &CPU::Opcodes[*instr];
    U8 additional_cycles = 0; //for conditional instructions

    //TODO: fix this variable
    //      0x0000 is a valid address to jump to, so it can't be used as a sentinel
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
        r->set_halfcarry_flag(add_will_halfcarry((REGISTER), 1)); \
        REGISTER += 1; \
        r->set_zero_flag(((REGISTER) == 0)); \
        r->set_subtract_flag(0); \
    } while(0)

        case 0x04: // INC B (Z 0 H -)
            INC_R_IMPL(r->b);
            break;

// decrements a single 8bit register
// assumes (Z 1 H -)
#define DEC_R_IMPL(REGISTER) \
    do { \
        r->set_halfcarry_flag(sub_will_halfborrow((REGISTER), 1));\
        REGISTER -= 1; \
        r->set_zero_flag(((REGISTER) == 0)); \
        r->set_subtract_flag(1); \
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
            r->set_zero_flag(0);
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            r->set_carry_flag(high_bit); // high bit goes into carry
            break;}

        case 0x08: // LD (a16),SP (- - - -)
            emu_mem_write(emu, DIRECT_U16, r->sp);
            break;

// assumes (- 0 H C)
#define ADD_HL_IMPL(OPERAND) \
    do { \
        U16 op = OPERAND; \
        r->set_halfcarry_flag(u16_add_will_halfcarry(r->hl, op)); \
        r->set_halfcarry_flag(u16_add_will_carry(r->hl, op)); \
        r->hl += op; \
        r->set_subtract_flag(0); \
    } while(0)

        case 0x09: // ADD HL,BC (- 0 H C)
            ADD_HL_IMPL(r->bc);
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
            r->set_zero_flag(0);
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            r->set_carry_flag(low_bit);
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
            U8 old_carry_low_bit = (r->carry_flag() ? 0x01 : 0x00);
            U8 old_high_bit = (r->a & 0x80);
            r->a = (r->a << 1) | old_carry_low_bit;
            r->set_zero_flag(0);
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            r->set_carry_flag(old_high_bit);
            break;}

        case 0x18: // JR r8 (- - - -)
            jump_to_addr = r->pc + (U16)DIRECT_U8;
            break;

        case 0x19: // ADD HL,DE (- 0 H C)
            ADD_HL_IMPL(r->de);
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
            U8 old_carry = (r->carry_flag() ? 0x80 : 0x00);
            r->a = (r->a >> 1) | old_carry;
            r->set_zero_flag(0);
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            r->set_carry_flag(old_low_bit);
        break;}

        case 0x20: // JR NZ,r8 (- - - -)
            if(!r->zero_flag()){
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
            if((r->a & 0x0F) > 0x09 || r->halfcarry_flag()){
                if(r->subtract_flag()){
                    r->a -= 0x06;
                } else {
                    r->a += 0x06;
                }
            }

            if((r->a & 0xF0) > 0x90 || r->carry_flag()){
                if(r->subtract_flag()){
                    r->a -= 0x60;
                } else {
                    r->a += 0x60;
                }
                r->set_carry_flag(1);
            } else {
                r->set_carry_flag(0);
            }

            r->set_zero_flag((r->a == 0));
            r->set_halfcarry_flag(0);
            break;}

        case 0x28: // JR Z,r8 (- - - -)
            if(r->zero_flag()){
                additional_cycles = 4;
                jump_to_addr = r->pc + DIRECT_U8;
            }
            break;

        case 0x29: // ADD HL,HL (- 0 H C)
            ADD_HL_IMPL(r->hl);
            break;

        case 0x2A: // LD A,(HL+) (- - - -)
            r->a = READ8_HL;
            r->hl += 1;
            break;

        case 0x2B: // DEC HL (- - - -)
            r->hl -= 1;
            break;

        case 0x2C: // INC L (Z 0 H -)
            INC_R_IMPL(r->l);
            break;

        case 0x2D: // DEC L (Z 1 H -)
            DEC_R_IMPL(r->l);
            break;

        case 0x2E: // LD L,d8 (- - - -)
            r->l = DIRECT_U8;
            break;

        case 0x2F: // CPL (- 1 1 -)
            r->a = ~(r->a);
            r->set_subtract_flag(1);
            r->set_halfcarry_flag(1);
            break;

        case 0x30: // JR NC,r8 (- - - -)
            if(!r->carry_flag()){
                additional_cycles = 4;
                jump_to_addr = r->pc + DIRECT_U8;
            }
            break;

        case 0x31: // LD SP,d16 (- - - -)
            r->sp = DIRECT_U16;
            break;

        case 0x32: // LD (HL-),A (- - - -)
            emu_mem_write(emu, r->hl, r->a);
            r->hl -= 1;
            break;

        case 0x33: // INC SP (- - - -)
            r->sp += 1;
            break;

        case 0x34:{// INC (HL) (Z 0 H -)
            U8 old_value = READ8_HL;
            U8 new_value = old_value + 1;
            emu_mem_write(emu, r->hl, new_value);
            r->set_zero_flag((new_value == 0));
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(add_will_halfcarry(old_value, 1));
            break;}

        case 0x35:{// DEC (HL) (Z 1 H -)
            U8 old_value = READ8_HL;
            U8 new_value = old_value - 1;
            emu_mem_write(emu, r->hl, new_value);
            r->set_zero_flag((new_value == 0));
            r->set_subtract_flag(1);
            r->set_halfcarry_flag(sub_will_halfborrow(old_value, 1));
            break;}

        case 0x36: // LD (HL),d8 (- - - -)
            emu_mem_write(emu, r->hl, DIRECT_U8);
            break;

        case 0x37: // SCF (- 0 0 1)
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            r->set_carry_flag(1);
            break;

        case 0x38: // JR C,r8 (- - - -)
            if(r->carry_flag()){
                additional_cycles = 4;
                jump_to_addr = r->pc + DIRECT_U8;
            }
            break;

        case 0x39: // ADD HL,SP (- 0 H C)
            ADD_HL_IMPL(r->sp);
            break;

        case 0x3A: // LD A,(HL-) (- - - -)
            r->a = READ8_HL;
            r->hl -= 1;
            break;

        case 0x3B: // DEC SP (- - - -)
            r->sp -= 1;
            break;

        case 0x3C: // INC A (Z 0 H -)
            INC_R_IMPL(r->a);
            break;

        case 0x3D: // DEC A (Z 1 H -)
            DEC_R_IMPL(r->a);
            break;

        case 0x3E: // LD A,d8 (- - - -)
            r->a = DIRECT_U8;
            break;

        case 0x3F: // CCF (- 0 0 C)
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            r->set_carry_flag(!r->carry_flag());
            break;

        case 0x40: // LD B,B (- - - -)
            // NOP
            break;
        case 0x41: // LD B,C (- - - -)
            r->b = r->c;
            break;
        case 0x42: // LD B,D (- - - -)
            r->b = r->d;
            break;
        case 0x43: // LD B,E (- - - -)
            r->b = r->e;
            break;
        case 0x44: // LD B,H (- - - -)
            r->b = r->h;
            break;
        case 0x45: // LD B,L (- - - -)
            r->b = r->l;
            break;
        case 0x46: // LD B,(HL) (- - - -)
            r->b = READ8_HL;
            break;
        case 0x47: // LD B,A (- - - -)
            r->b = r->a;
            break;

        case 0x48: // LD C,B (- - - -)
            r->c = r->b;
            break;
        case 0x49: // LD C,C (- - - -)
            // NOP
            break;
        case 0x4A: // LD C,D (- - - -)
            r->c = r->d;
            break;
        case 0x4B: // LD C,E (- - - -)
            r->c = r->e;
            break;
        case 0x4C: // LD C,H (- - - -)
            r->c = r->h;
            break;
        case 0x4D: // LD C,L (- - - -)
            r->c = r->l;
            break;
        case 0x4E: // LD C,(HL) (- - - -)
            r->c = READ8_HL;
            break;
        case 0x4F: // LD C,A (- - - -)
            r->c = r-> a;
            break;

        case 0x50: // LD D,B (- - - -)
            r->d = r->b;
            break;
        case 0x51: // LD D,C (- - - -)
            r->d = r->c;
            break;
        case 0x52: // LD D,D (- - - -)
            // NOP
            break;
        case 0x53: // LD D,E (- - - -)
            r->d = r->e;
            break;
        case 0x54: // LD D,H (- - - -)
            r->d = r->h;
            break;
        case 0x55: // LD D,L (- - - -)
            r->d = r->l;
            break;
        case 0x56: // LD D,(HL) (- - - -)
            r->d = READ8_HL;
            break;
        case 0x57: // LD D,A (- - - -)
            r->d = r->a;
            break;

        case 0x58: // LD E,B (- - - -)
            r->e = r->b;
            break;
        case 0x59: // LD E,C (- - - -)
            r->e = r->c;
            break;
        case 0x5A: // LD E,D (- - - -)
            r->e = r->d;
            break;
        case 0x5B: // LD E,E (- - - -)
            // NOP
            break;
        case 0x5C: // LD E,H (- - - -)
            r->e = r->h;
            break;
        case 0x5D: // LD E,L (- - - -)
            r->e = r->l;
            break;
        case 0x5E: // LD E,(HL) (- - - -)
            r->e = READ8_HL;
            break;
        case 0x5F: // LD E,A (- - - -)
            r->e = r->a;
            break;

        case 0x60: // LD H,B (- - - -)
            r->h = r->b;
            break;
        case 0x61: // LD H,C (- - - -)
            r->h = r->c;
            break;
        case 0x62: // LD H,D (- - - -)
            r->h = r->d;
            break;
        case 0x63: // LD H,E (- - - -)
            r->h = r->e;
            break;
        case 0x64: // LD H,H (- - - -)
            // NOP
            break;
        case 0x65: // LD H,L (- - - -)
            r->h = r->l;
            break;
        case 0x66: // LD H,(HL) (- - - -)
            r->h = READ8_HL;
            break;
        case 0x67: // LD H,A (- - - -)
            r->h = r->a;
            break;

        case 0x68: // LD L,B (- - - -)
            r->l = r->b;
            break;
        case 0x69: // LD L,C (- - - -)
            r->l = r->c;
            break;
        case 0x6A: // LD L,D (- - - -)
            r->l = r->d;
            break;
        case 0x6B: // LD L,E (- - - -)
            r->l = r->e;
            break;
        case 0x6C: // LD L,H (- - - -)
            r->l = r->h;
            break;
        case 0x6D: // LD L,L (- - - -)
            // NOP
            break;
        case 0x6E: // LD L,(HL) (- - - -)
            r->l = READ8_HL;
            break;
        case 0x6F: // LD L,A (- - - -)
            r->l = r->a;
            break;

        case 0x70: // LD (HL),B (- - - -)
            emu_mem_write(emu, r->hl, r->b);
            break;
        case 0x71: // LD (HL),C (- - - -)
            emu_mem_write(emu, r->hl, r->c);
            break;
        case 0x72: // LD (HL),D (- - - -)
            emu_mem_write(emu, r->hl, r->d);
            break;
        case 0x73: // LD (HL),E (- - - -)
            emu_mem_write(emu, r->hl, r->e);
            break;
        case 0x74: // LD (HL),H (- - - -)
            emu_mem_write(emu, r->hl, r->h);
            break;
        case 0x75: // LD (HL),L (- - - -)
            emu_mem_write(emu, r->hl, r->l);
            break;

        case 0x76: // HALT (- - - -)
            //TODO: here
            break;

        case 0x77: // LD (HL),A (- - - -)
            emu_mem_write(emu, r->hl, r->a);
            break;
        case 0x78: // LD A,B (- - - -)
            r->a = r->b;
            break;
        case 0x79: // LD A,C (- - - -)
            r->a = r->c;
            break;
        case 0x7A: // LD A,D (- - - -)
            r->a = r->d;
            break;
        case 0x7B: // LD A,E (- - - -)
            r->a = r->e;
            break;
        case 0x7C: // LD A,H (- - - -)
            r->a = r->h;
            break;
        case 0x7D: // LD A,L (- - - -)
            r->a = r->l;
            break;
        case 0x7E: // LD A,(HL) (- - - -)
            r->a = READ8_HL;
            break;
        case 0x7F: // LD A,A (- - - -)
            // NOP
            break;

// assumes (Z 0 H C)
#define ADD_A_IMPL(OPERAND) \
    do { \
        U8 op = OPERAND; \
        r->set_subtract_flag(0); \
        r->set_halfcarry_flag(add_will_halfcarry(r->a, op)); \
        r->set_carry_flag(add_will_carry(r->a, op)); \
        r->a += op; \
        r->set_zero_flag((r->a == 0)); \
    } while(0)

        case 0x80: // ADD A,B (Z 0 H C)
            ADD_A_IMPL(r->b);
            break;
        case 0x81: // ADD A,C (Z 0 H C)
            ADD_A_IMPL(r->c);
            break;
        case 0x82: // ADD A,D (Z 0 H C)
            ADD_A_IMPL(r->d);
            break;
        case 0x83: // ADD A,E (Z 0 H C)
            ADD_A_IMPL(r->e);
            break;
        case 0x84: // ADD A,H (Z 0 H C)
            ADD_A_IMPL(r->h);
            break;
        case 0x85: // ADD A,L (Z 0 H C)
            ADD_A_IMPL(r->l);
            break;
        case 0x86: // ADD A,(HL) (Z 0 H C)
            ADD_A_IMPL(READ8_HL);
            break;
        case 0x87: // ADD A,A (Z 0 H C)
            ADD_A_IMPL(r->a);
            break;

//assumes (Z 0 H C)
#define ADC_A_IMPL(OPERAND) \
    ADD_A_IMPL((OPERAND) + (r->carry_flag() ? 1 : 0))

        case 0x88: // ADC A,B (Z 0 H C)
            ADC_A_IMPL(r->b);
            break;
        case 0x89: // ADC A,C (Z 0 H C)
            ADC_A_IMPL(r->c);
            break;
        case 0x8A: // ADC A,D (Z 0 H C)
            ADC_A_IMPL(r->d);
            break;
        case 0x8B: // ADC A,E (Z 0 H C)
            ADC_A_IMPL(r->e);
            break;
        case 0x8C: // ADC A,H (Z 0 H C)
            ADC_A_IMPL(r->h);
            break;
        case 0x8D: // ADC A,L (Z 0 H C)
            ADC_A_IMPL(r->l);
            break;
        case 0x8E: // ADC A,(HL) (Z 0 H C)
            ADC_A_IMPL(READ8_HL);
            break;
        case 0x8F: // ADC A,A (Z 0 H C)
            ADC_A_IMPL(r->a);
            break;

//assumes (Z 1 H C)
#define SUB_A_IMPL(OPERAND) \
    do { \
        U8 op = OPERAND; \
        r->set_subtract_flag(1); \
        r->set_halfcarry_flag(sub_will_halfborrow(r->a, op)); \
        r->set_carry_flag(sub_will_borrow(r->a, op)); \
        r->a -= op; \
        r->set_zero_flag((r->a == 0)); \
    } while(0)

        case 0x90: // SUB B (Z 1 H C)
            SUB_A_IMPL(r->b);
            break;
        case 0x91: // SUB C (Z 1 H C)
            SUB_A_IMPL(r->c);
            break;
        case 0x92: // SUB D (Z 1 H C)
            SUB_A_IMPL(r->d);
            break;
        case 0x93: // SUB E (Z 1 H C)
            SUB_A_IMPL(r->e);
            break;
        case 0x94: // SUB H (Z 1 H C)
            SUB_A_IMPL(r->h);
            break;
        case 0x95: // SUB L (Z 1 H C)
            SUB_A_IMPL(r->l);
            break;
        case 0x96: // SUB (HL) (Z 1 H C)
            SUB_A_IMPL(READ8_HL);
            break;
        case 0x97: // SUB A (Z 1 H C)
            SUB_A_IMPL(r->a);
            break;

#define SBC_A_IMPL(OPERAND) \
    SUB_A_IMPL((OPERAND) - (r->carry_flag() ? 1 : 0))

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

//assumes (Z 0 1 0)
#define AND_A_IMPL(OPERAND) \
    do { \
        r->a &= (OPERAND); \
        r->set_zero_flag((r->a == 0)); \
        r->set_subtract_flag(0); \
        r->set_halfcarry_flag(1); \
        r->set_carry_flag(0); \
    } while(0)

        case 0xA0: // AND B (Z 0 1 0)
            AND_A_IMPL(r->b);
            break;
        case 0xA1: // AND C (Z 0 1 0)
            AND_A_IMPL(r->c);
            break;
        case 0xA2: // AND D (Z 0 1 0)
            AND_A_IMPL(r->d);
            break;
        case 0xA3: // AND E (Z 0 1 0)
            AND_A_IMPL(r->e);
            break;
        case 0xA4: // AND H (Z 0 1 0)
            AND_A_IMPL(r->h);
            break;
        case 0xA5: // AND L (Z 0 1 0)
            AND_A_IMPL(r->l);
            break;
        case 0xA6: // AND (HL) (Z 0 1 0)
            AND_A_IMPL(READ8_HL);
            break;
        case 0xA7: // AND A (Z 0 1 0)
            AND_A_IMPL(r->a);
            break;

//assumes (Z 0 0 0)
#define XOR_A_IMPL(OPERAND) \
    do { \
        r->a ^= (OPERAND); \
        r->set_zero_flag((r->a == 0)); \
        r->set_subtract_flag(0); \
        r->set_halfcarry_flag(0); \
        r->set_carry_flag(0); \
    } while(0)

        case 0xA8: // XOR B (Z 0 0 0)
            XOR_A_IMPL(r->b);
            break;
        case 0xA9: // XOR C (Z 0 0 0)
            XOR_A_IMPL(r->c);
            break;
        case 0xAA: // XOR D (Z 0 0 0)
            XOR_A_IMPL(r->d);
            break;
        case 0xAB: // XOR E (Z 0 0 0)
            XOR_A_IMPL(r->e);
            break;
        case 0xAC: // XOR H (Z 0 0 0)
            XOR_A_IMPL(r->h);
            break;
        case 0xAD: // XOR L (Z 0 0 0)
            XOR_A_IMPL(r->l);
            break;
        case 0xAE: // XOR (HL) (Z 0 0 0)
            XOR_A_IMPL(READ8_HL);
            break;
        case 0xAF: // XOR A (Z 0 0 0)
            XOR_A_IMPL(r->a);
            break;

//assumes (Z 0 0 0)
#define OR_A_IMPL(OPERAND) \
    do { \
        r->a |= (OPERAND); \
        r->set_zero_flag((r->a == 0)); \
        r->set_subtract_flag(0); \
        r->set_halfcarry_flag(0); \
        r->set_carry_flag(0); \
    } while(0)

        case 0xB0: // OR B (Z 0 0 0)
            OR_A_IMPL(r->b);
            break;
        case 0xB1: // OR C (Z 0 0 0)
            OR_A_IMPL(r->c);
            break;
        case 0xB2: // OR D (Z 0 0 0)
            OR_A_IMPL(r->d);
            break;
        case 0xB3: // OR E (Z 0 0 0)
            OR_A_IMPL(r->e);
            break;
        case 0xB4: // OR H (Z 0 0 0)
            OR_A_IMPL(r->h);
            break;
        case 0xB5: // OR L (Z 0 0 0)
            OR_A_IMPL(r->l);
            break;
        case 0xB6: // OR (HL) (Z 0 0 0)
            OR_A_IMPL(READ8_HL);
            break;
        case 0xB7: // OR A (Z 0 0 0)
            OR_A_IMPL(r->a);
            break;

/*
 Compare A with n. This is basically an A - n subtraction instruction
 but the results are thrown away.
 
 assumes (Z 1 H C)
*/
#define CP_A_IMPL(OPERAND) \
    do { \
        U8 op = (OPERAND); \
        r->set_zero_flag(((r->a - op) == 0)); \
        r->set_subtract_flag(1); \
        r->set_halfcarry_flag(sub_will_halfborrow(r->a, op)); \
        r->set_carry_flag(sub_will_borrow(r->a, op)); \
    } while(0)

        case 0xB8: // CP B (Z 1 H C)
            CP_A_IMPL(r->b);
            break;
        case 0xB9: // CP C (Z 1 H C)
            CP_A_IMPL(r->c);
            break;
        case 0xBA: // CP D (Z 1 H C)
            CP_A_IMPL(r->d);
            break;
        case 0xBB: // CP E (Z 1 H C)
            CP_A_IMPL(r->e);
            break;
        case 0xBC: // CP H (Z 1 H C)
            CP_A_IMPL(r->h);
            break;
        case 0xBD: // CP L (Z 1 H C)
            CP_A_IMPL(r->l);
            break;
        case 0xBE: // CP (HL) (Z 1 H C)
            CP_A_IMPL(READ8_HL);
            break;
        case 0xBF: // CP A (Z 1 H C)
            CP_A_IMPL(r->a);
            break;

//TODO: implement this
//Pop two bytes from stack & jump to that address.
#define RET_IMPL \
    do { \
        jump_to_addr = emu_stack_pop(emu); \
    } while(0)

        case 0xC0: // RET NZ (- - - -)
            if(!r->zero_flag()){
                additional_cycles += 12;
                RET_IMPL;
            }
            break;

        case 0xC1: // POP BC (- - - -)
            r->bc = emu_stack_pop(emu);
            break;

        case 0xC2: // JP NZ,a16 (- - - -)
            if(!r->zero_flag()){
                additional_cycles = 4;
                jump_to_addr = DIRECT_U16;
            }
            break;

        case 0xC3: // JP a16 (- - - -)
            jump_to_addr = DIRECT_U16;
            break;

#define CALL_IMPL(ADDRESS) \
    do { \
        emu_stack_push(emu, r->pc + opcode->byte_length); \
        jump_to_addr = (ADDRESS); \
    } while(0)


#define CONDITIONAL_CALL_IMPL(COND, ADDRESS) \
    do {\
        if(COND){ \
            additional_cycles = 12; \
            CALL_IMPL(ADDRESS); \
        } \
    } while(0)


        case 0xC4: // CALL NZ,a16 (- - - -)
            CONDITIONAL_CALL_IMPL(!r->zero_flag(), DIRECT_U16);
            break;

        case 0xC5: // PUSH BC (- - - -)
            emu_stack_push(emu, r->bc);
            break;

        case 0xC6: // ADD A,d8 (Z 0 H C)
            ADD_A_IMPL(DIRECT_U8);
            break;

        case 0xC7: // RST 00H (- - - -)
            emu_stack_push(emu, r->pc);
            jump_to_addr = 0x0000;
            break;

        case 0xC8: // RET Z (- - - -)
            if(r->zero_flag()){
                additional_cycles = 12;
                RET_IMPL;
            }
            break;

        case 0xC9: // RET (- - - -)
            RET_IMPL;
            break;

        case 0xCA: // JP Z,a16 (- - - -)
            if(r->zero_flag()){
                additional_cycles = 4;
                jump_to_addr = DIRECT_U16;
            }
            break;

        case 0xCB: // PREFIX CB
            //TODO: implement the other 256 opcodes with the CB prefix
            break;

        case 0xCC: // CALL Z,a16 (- - - -)
            CONDITIONAL_CALL_IMPL(r->zero_flag(), DIRECT_U16);
            break;

        case 0xCD: // CALL a16 (- - - -)
            CALL_IMPL(DIRECT_U16);
            break;

        case 0xCE: // ADC A,d8 (Z 0 H C)
            ADC_A_IMPL(DIRECT_U8);
            break;

        case 0xCF: // RST 08H (- - - -)
            emu_stack_push(emu, r->pc);
            jump_to_addr = 0x0008;
            break;

        case 0xD0: // RET NC (- - - -)
            if(!r->carry_flag()){
                additional_cycles = 12;
                RET_IMPL;
            }
            break;

        case 0xD1: // POP DE (- - - -)
            r->de = emu_stack_pop(emu);
            break;

        case 0xD2: // JP NC,a16 (- - - -)
            if(!r->carry_flag()){
                additional_cycles = 4;
                jump_to_addr = DIRECT_U16;
            }
            break;

        case 0xD3: // INVALID_INSTRUCTION
            break;

        case 0xD4: // CALL NC,a16 (- - - -)
            CONDITIONAL_CALL_IMPL(!r->carry_flag(), DIRECT_U16);
            break;

        case 0xD5: // PUSH DE (- - - -)
            emu_stack_push(emu, r->de);
            break;

        case 0xD6: // SUB d8 (Z 1 H C)
            SUB_A_IMPL(DIRECT_U8);
            break;

        case 0xD7: // RST 10H (- - - -)
            emu_stack_push(emu, r->pc);
            jump_to_addr = 0x0010;
            break;

        case 0xD8: // RET C (- - - -)
            if(r->carry_flag()){
                additional_cycles = 12;
                RET_IMPL;
            }
            break;

        case 0xD9: // RETI (- - - -)
            RET_IMPL;
            //TODO: enable interrupts here
            break;

        case 0xDA: // JP C,a16 (- - - -)
            if(r->carry_flag()){
                additional_cycles = 4;
                jump_to_addr = DIRECT_U16;
            }
            break;

        case 0xDB: // INVALID_INSTRUCTION
            break;

        case 0xDC: // CALL C,a16 (- - - -)
            CONDITIONAL_CALL_IMPL(r->carry_flag(), DIRECT_U16);
            break;

        case 0xDD: // INVALID_INSTRUCTION
            break;

        case 0xDE: // SBC A,d8 (Z 1 H C)
            SBC_A_IMPL(DIRECT_U8);
            break;

        case 0xDF: // RST 18H (- - - -)
            emu_stack_push(emu, r->pc);
            jump_to_addr = 0x0018;
            break;

        case 0xE0: // LDH (a8),A (- - - -)
            emu_mem_write(emu, 0xFF00 + (U16)DIRECT_U8, r->a);
            break;

        case 0xE1: // POP HL (- - - -)
            r->hl = emu_stack_pop(emu);
            break;

        case 0xE2: // LD (C),A (- - - -)
            emu_mem_write(emu, 0xFF00 + (U16)r->c, r->a);
            break;

        case 0xE3: // INVALID_INSTRUCTION
            break;
        case 0xE4: // INVALID_INSTRUCTION
            break;

        case 0xE5: // PUSH HL (- - - -)
            emu_stack_push(emu, r->hl);
            break;

        case 0xE6: // AND d8 (Z 0 1 0)
            AND_A_IMPL(DIRECT_U8);
            break;

        case 0xE7: // RST 20H (- - - -)
            emu_stack_push(emu, r->pc);
            jump_to_addr = 0x0020;
            break;

        case 0xE8:{// ADD SP,r8 (0 0 H C)
            U16 operand = DIRECT_U8;
            r->set_zero_flag(0);
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(u16_add_will_halfcarry(r->sp, operand));
            r->set_carry_flag(u16_add_will_carry(r->sp, operand));
            r->sp += operand;
            break;}

        case 0xE9: // JP (HL) (- - - -)
            jump_to_addr = READ8_HL;
            break;

        case 0xEA: // LD (a16),A (- - - -)
            emu_mem_write(emu, DIRECT_U16, r->a);
            break;

        case 0xEB: // INVALID_INSTRUCTION
            break;
        case 0xEC: // INVALID_INSTRUCTION
            break;
        case 0xED: // INVALID_INSTRUCTION
            break;

        case 0xEE: // XOR d8 (Z 0 0 0)
            XOR_A_IMPL(DIRECT_U8);
            break;

        case 0xEF: // RST 28H (- - - -)
            emu_stack_push(emu, r->pc);
            jump_to_addr = 0x0028;
            break;

        case 0xF0: // LDH A,(a8) (- - - -)
            r->a = emu_mem_read<U8>(emu, 0xFF00 + (U16)DIRECT_U8);
            break;

        case 0xF1: // POP AF (Z N H C)
            // all flags set by virtue of setting r->f
            r->af = emu_stack_pop(emu);
            break;

        case 0xF2: // LD A,(C)
            r->a = emu_mem_read<U8>(emu, 0xFF00 + (U16)r->c);
            break;

        case 0xF3: // DI (- - - -)
            /*
             TODO: here
             
             This instruction disables interrupts but not immediately. 
             Interrupts are disabled after instruction after DI 
             is executed.
             */
            break;

        case 0xF4: // INVALID_INSTRUCTION
            break;

        case 0xF5: // PUSH AF (- - - -)
            emu_stack_push(emu, r->af);
            break;

        case 0xF6: // OR d8 (Z 0 0 0)
            OR_A_IMPL(DIRECT_U8);
            break;

        case 0xF7: // RST 30H (- - - -)
            emu_stack_push(emu, r->pc);
            jump_to_addr = 0x0030;
            break;

        case 0xF8:{// LD HL,SP+r8 (0 0 H C)
            U16 relative_address = DIRECT_U8;
            r->set_zero_flag(0);
            r->set_subtract_flag(0);
            //TODO: wtf are these carry flags supposed to be?
            r->set_halfcarry_flag(u16_add_will_halfcarry(r->sp, relative_address));
            r->set_carry_flag(u16_add_will_carry(r->sp, relative_address));
            r->hl = r->sp + relative_address;
            break;}

        case 0xF9: // LD SP,HL (- - - -)
            r->sp = r->hl;
            break;

        case 0xFA: // LD A,(a16) (- - - -)
            r->a = emu_mem_read<U8>(emu, DIRECT_U16);
            break;

        case 0xFB: // EI
            /*
             TODO: here
             
             Enable interrupts. This intruction enables interrupts
             but not immediately. Interrupts are enabled after instruction after EI
             is executed.
             */
            break;

        case 0xFC: // INVALID_INSTRUCTION
            break;
        case 0xFD: // INVALID_INSTRUCTION
            break;

        case 0xFE: // CP d8 (Z 1 H C)
            CP_A_IMPL(DIRECT_U8);
            break;

        case 0xFF: // RST 38H
            emu_stack_push(emu, r->pc);
            jump_to_addr = 0x0038;
            break;
    }

    //TODO: fix this - 0 is a valid address to jump to
    if(jump_to_addr > 0){
        // jump to an arbitrary address
        r->pc = jump_to_addr;
    } else {
        // step to next instruction
        r->pc += opcode->byte_length;
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

void cart_fread(Cart::Cart* cart, const char* filename) {
    FILE* f = fopen(filename, "rb");
    assert(f);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    assert(size < Cart::MaxSize);
    rewind(f);

    fread(cart, size, 1, f);
    fclose(f);
}

void cart_get_title(Cart::Cart* cart, char* out_title) {
    strncpy(out_title, (const char*)&(cart->header.game_title), Cart::TitleSize - 1);
    out_title[Cart::TitleSize - 1] = 0;
}

void test(Emulator* emu) {
    // check typedef'd sizes
    assert(sizeof(U8) == 1);
    assert(sizeof(U16) == 2);
    assert(sizeof(U32) == 4);

    // check 16bit register byte order
    emu->registers.hl = 0xABCD;
    assert(emu->registers.h == 0xAB);
    assert(emu->registers.l == 0xCD);
    emu->registers.hl = 0;

    // check byte order of U16 (least significant first)
    U16 u16 = 0xABCD;
    U8* u16p = (U8*)(void*)&u16;
    assert(u16p[0] == 0xCD);
    assert(u16p[1] == 0xAB);
}

int main(int argc, const char * argv[]) {
    assert(argc == 2);

    Emulator* emu = new Emulator;
    emu_init(emu);
    cart_fread(&emu->cart, argv[1]);

    test(emu);
}

