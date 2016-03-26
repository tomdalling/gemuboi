//
//  emulator.cpp
//  gemuboi
//
//  Created by Tom on 24/09/2015.
//
//

#include <cassert>
#include <cstdio>
#include <cstring>

#include "emulator.hpp"

const U16 BootstrapRomSize = 256;
const U16 BootstrapRom_Enabled = 0x00; // BS ROM is readable
//const U16 BootstrapRom_Disabled = 0x01; // BS ROM not readable (replaced by cartridge ROM)
const U8 BootstrapRom[BootstrapRomSize] = {
    0x31, 0xfe, 0xff, 0xaf, 0x21, 0xff, 0x9f, 0x32, 0xcb, 0x7c, 0x20, 0xfb,
    0x21, 0x26, 0xff, 0x0e, 0x11, 0x3e, 0x80, 0x32, 0xe2, 0x0c, 0x3e, 0xf3,
    0xe2, 0x32, 0x3e, 0x77, 0x77, 0x3e, 0xfc, 0xe0, 0x47, 0x11, 0x04, 0x01,
    0x21, 0x10, 0x80, 0x1a, 0xcd, 0x95, 0x00, 0xcd, 0x96, 0x00, 0x13, 0x7b,
    0xfe, 0x34, 0x20, 0xf3, 0x11, 0xd8, 0x00, 0x06, 0x08, 0x1a, 0x13, 0x22,
    0x23, 0x05, 0x20, 0xf9, 0x3e, 0x19, 0xea, 0x10, 0x99, 0x21, 0x2f, 0x99,
    0x0e, 0x0c, 0x3d, 0x28, 0x08, 0x32, 0x0d, 0x20, 0xf9, 0x2e, 0x0f, 0x18,
    0xf3, 0x67, 0x3e, 0x64, 0x57, 0xe0, 0x42, 0x3e, 0x91, 0xe0, 0x40, 0x04,
    0x1e, 0x02, 0x0e, 0x0c, 0xf0, 0x44, 0xfe, 0x90, 0x20, 0xfa, 0x0d, 0x20,
    0xf7, 0x1d, 0x20, 0xf2, 0x0e, 0x13, 0x24, 0x7c, 0x1e, 0x83, 0xfe, 0x62,
    0x28, 0x06, 0x1e, 0xc1, 0xfe, 0x64, 0x20, 0x06, 0x7b, 0xe2, 0x0c, 0x3e,
    0x87, 0xe2, 0xf0, 0x42, 0x90, 0xe0, 0x42, 0x15, 0x20, 0xd2, 0x05, 0x20,
    0x4f, 0x16, 0x20, 0x18, 0xcb, 0x4f, 0x06, 0x04, 0xc5, 0xcb, 0x11, 0x17,
    0xc1, 0xcb, 0x11, 0x17, 0x05, 0x20, 0xf5, 0x22, 0x23, 0x22, 0x23, 0xc9,
    0xce, 0xed, 0x66, 0x66, 0xcc, 0x0d, 0x00, 0x0b, 0x03, 0x73, 0x00, 0x83,
    0x00, 0x0c, 0x00, 0x0d, 0x00, 0x08, 0x11, 0x1f, 0x88, 0x89, 0x00, 0x0e,
    0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd, 0xd9, 0x99, 0xbb, 0xbb, 0x67, 0x63,
    0x6e, 0x0e, 0xec, 0xcc, 0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e,
    0x3c, 0x42, 0xb9, 0xa5, 0xb9, 0xa5, 0x42, 0x3c, 0x21, 0x04, 0x01, 0x11,
    0xa8, 0x00, 0x1a, 0x13, 0xbe, 0x20, 0xfe, 0x23, 0x7d, 0xfe, 0x34, 0x20,
    0xf5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xfb, 0x86, 0x20, 0xfe,
    0x3e, 0x01, 0xe0, 0x50
};

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

// (Z 0 H C)
void add_a_impl(U8 operand, BOOL32 add_carry, CPU::Registers* r) {
    if(add_carry && r->carry_flag())
        operand += 1;

    r->set_subtract_flag(0);
    r->set_halfcarry_flag(add_will_halfcarry(r->a, operand));
    r->set_carry_flag(add_will_carry(r->a, operand));
    r->a += operand;
    r->set_zero_flag(r->a == 0);
}

// (Z 1 H C)
void sub_a_impl(U8 operand, BOOL32 sub_carry, CPU::Registers* r) {
    if(sub_carry && r->carry_flag())
        operand += 1;

    r->set_subtract_flag(1);
    r->set_halfcarry_flag(sub_will_halfborrow(r->a, operand));
    r->set_carry_flag(sub_will_borrow(r->a, operand));
    r->a -= operand;
    r->set_zero_flag(r->a == 0);
}

// (Z 1 H C)
void cp_a_impl(U8 operand, CPU::Registers* r) {
    /*
     Compare A with n. This is basically an A - n subtraction instruction
     but the results are thrown away.
     */
    r->set_zero_flag((r->a - operand) == 0);
    r->set_subtract_flag(1);
    r->set_halfcarry_flag(sub_will_halfborrow(r->a, operand));
    r->set_carry_flag(sub_will_borrow(r->a, operand));
}

// (Z 0 0 0)
void and_a_impl(U8 operand, CPU::Registers* r) {
    r->a &= operand;
    r->set_zero_flag(r->a == 0);
    r->set_subtract_flag(0);
    r->set_halfcarry_flag(1);
    r->set_carry_flag(0);
}

// (Z 0 0 0)
void or_a_impl(U8 operand, CPU::Registers* r) {
    r->a |= operand;
    r->set_zero_flag(r->a == 0);
    r->set_subtract_flag(0);
    r->set_halfcarry_flag(0);
    r->set_carry_flag(0);
}

// (Z 0 0 0)
void xor_a_impl(U8 operand, CPU::Registers* r) {
    r->a ^= operand;
    r->set_zero_flag(r->a == 0);
    r->set_subtract_flag(0);
    r->set_halfcarry_flag(0);
    r->set_carry_flag(0);
}

// (Z 0 H -)
void inc_u8_impl(U8* in_out_value, CPU::Registers* r) {
    r->set_halfcarry_flag(add_will_halfcarry(*in_out_value, 1));
    *in_out_value += 1;
    r->set_zero_flag(*in_out_value == 0);
    r->set_subtract_flag(0);
}

// (Z 1 H -)
void dec_u8_impl(U8* in_out_value, CPU::Registers* r){
    r->set_halfcarry_flag(sub_will_halfborrow(*in_out_value, 1));
    *in_out_value -= 1;
    r->set_zero_flag(*in_out_value == 0);
    r->set_subtract_flag(1);
}

// (- 0 H C)
void add_hl_impl(U16 operand, CPU::Registers* r){
    r->set_subtract_flag(0);
    r->set_halfcarry_flag(u16_add_will_halfcarry(r->hl, operand));
    r->set_halfcarry_flag(u16_add_will_carry(r->hl, operand));
    r->hl += operand;
}

inline
void rlc(U8* in_out_value, CPU::Registers* registers) {
    /*
     Rotate left, copy to carry.
     Old high bit (7) becomes the new low bit (0).
     Old high bit (7) also gets stored in the carry flag.

             +---------------------------------+
             |                                 |
             |                                 v
     +---+   |   +---+---+---+---+---+---+---+---+
     | C |<--+---| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
     +---+       +---+---+---+---+---+---+---+---+

     */

    U8 old_high_bit = (*in_out_value & 0x80); //remember high bit
    *in_out_value = (*in_out_value << 1) | (old_high_bit >> 7);
    registers->set_carry_flag(old_high_bit);
}

inline
void rrc(U8* in_out_value, CPU::Registers* registers) {
    /*
     Rotate right, copy to carry.
     Old low bit (0) becomes the new high bit (7).
     Old low bit (0) to the carry flag.

       +-----+---------------------------------+
       |     |                                 |
       v     |                                 |
     +---+   |   +---+---+---+---+---+---+---+---+
     | C |   +-->| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
     +---+       +---+---+---+---+---+---+---+---+
     */

    U8 old_low_bit = (*in_out_value & 0x01);
    *in_out_value = (*in_out_value >> 1) | (old_low_bit << 7);
    registers->set_carry_flag(old_low_bit);
}

inline
void rr(U8* in_out_value, CPU::Registers* registers) {
    /*
     Rotate right, through carry flag.
     Old low bit (0) becomes the new carry flag.
     Old carry flag becomes the new high bit (7).

       +---------------------------------------+
       |                                       |
       v                                       |
     +---+       +---+---+---+---+---+---+---+---+
     | C |------>| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
     +---+       +---+---+---+---+---+---+---+---+
     */
    U8 old_low_bit = (*in_out_value & 0x01);
    U8 old_carry = (registers->carry_flag() ? 0x80 : 0x00);
    *in_out_value = (*in_out_value >> 1) | old_carry;
    registers->set_carry_flag(old_low_bit);
}

inline
void rl(U8* in_out_value, CPU::Registers* registers) {
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

    U8 old_carry = (registers->carry_flag() ? 0x01 : 0x00);
    U8 old_high_bit = (*in_out_value & 0x80);
    *in_out_value = (*in_out_value << 1) | old_carry;
    registers->set_carry_flag(old_high_bit);
}

U8 emu_standard_operand_read(Emulator* emu, U8 opcode) {
    CPU::Registers* r = &emu->registers;

    U8 lower_nibble = (opcode & 0x0F);
    switch(lower_nibble){
        case 0x00: case 0x08: return r->b;
        case 0x01: case 0x09: return r->c;
        case 0x02: case 0x0A: return r->d;
        case 0x03: case 0x0B: return r->e;
        case 0x04: case 0x0C: return r->h;
        case 0x05: case 0x0D: return r->l;
        case 0x06: case 0x0E: return emu->mem_read(r->hl);
        case 0x07: case 0x0F: return r->a;
        default:
            assert(0); //should never get here
            return 0;
    }
}

void emu_standard_operand_write(Emulator* emu, U8 opcode, U8 value) {
    CPU::Registers* r = &emu->registers;

    U8 lower_nibble = (opcode & 0x0F);
    switch(lower_nibble){
        case 0x00: case 0x08: r->b = value; break;
        case 0x01: case 0x09: r->c = value; break;
        case 0x02: case 0x0A: r->d = value; break;
        case 0x03: case 0x0B: r->e = value; break;
        case 0x04: case 0x0C: r->h = value; break;
        case 0x05: case 0x0D: r->l = value; break;
        case 0x06: case 0x0E: return emu->mem_write(r->hl, value); break;
        case 0x07: case 0x0F: r->a = value; break;
        default:
            assert(0); //should never get here
    }
}

void emu_cb_instruction(Emulator* emu, U8 cb_instr) {
    CPU::Registers* r = &emu->registers;
    U8 operand = emu_standard_operand_read(emu, cb_instr);

    switch(cb_instr){
        case 0x00: // RLC B (Z 0 0 C)
        case 0x01: // RLC C (Z 0 0 C)
        case 0x02: // RLC D (Z 0 0 C)
        case 0x03: // RLC E (Z 0 0 C)
        case 0x04: // RLC H (Z 0 0 C)
        case 0x05: // RLC L (Z 0 0 C)
        case 0x06: // RLC (HL) (Z 0 0 C)
        case 0x07: // RLC A (Z 0 0 C)
            // Rotate left. Old bit 7 to Carry flag.
            rlc(&operand, r);
            r->set_zero_flag(operand == 0);
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            break;

        case 0x08: // RRC B (Z 0 0 C)
        case 0x09: // RRC C (Z 0 0 C)
        case 0x0A: // RRC D (Z 0 0 C)
        case 0x0B: // RRC E (Z 0 0 C)
        case 0x0C: // RRC H (Z 0 0 C)
        case 0x0D: // RRC L (Z 0 0 C)
        case 0x0E: // RRC (HL) (Z 0 0 C)
        case 0x0F: // RRC A (Z 0 0 C)
            // Rotate right. Old bit 0 to Carry flag.
            rrc(&operand, r);
            r->set_zero_flag(operand == 0);
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            break;

        case 0x10: // RL B (Z 0 0 C)
        case 0x11: // RL C (Z 0 0 C)
        case 0x12: // RL D (Z 0 0 C)
        case 0x13: // RL E (Z 0 0 C)
        case 0x14: // RL H (Z 0 0 C)
        case 0x15: // RL L (Z 0 0 C)
        case 0x16: // RL (HL) (Z 0 0 C)
        case 0x17: // RL A (Z 0 0 C)
            //Rotate left through Carry flag.
            rl(&operand, r);
            r->set_zero_flag(operand == 0);
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            break;

        case 0x18: // RR B (Z 0 0 C)
        case 0x19: // RR C (Z 0 0 C)
        case 0x1A: // RR D (Z 0 0 C)
        case 0x1B: // RR E (Z 0 0 C)
        case 0x1C: // RR H (Z 0 0 C)
        case 0x1D: // RR L (Z 0 0 C)
        case 0x1E: // RR (HL) (Z 0 0 C)
        case 0x1F: // RR A (Z 0 0 C)
            //Rotate right through Carry flag.
            rr(&operand, r);
            r->set_zero_flag(operand == 0);
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            break;

        case 0x20: // SLA B (Z 0 0 C)
        case 0x21: // SLA C (Z 0 0 C)
        case 0x22: // SLA D (Z 0 0 C)
        case 0x23: // SLA E (Z 0 0 C)
        case 0x24: // SLA H (Z 0 0 C)
        case 0x25: // SLA L (Z 0 0 C)
        case 0x26: // SLA (HL) (Z 0 0 C)
        case 0x27:{// SLA A (Z 0 0 C)
            // Shift left into Carry. Low bit set to 0.
            U8 old_high_bit = (operand & 0x80);
            operand <<= 1;
            r->set_zero_flag(operand == 0);
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            r->set_carry_flag(old_high_bit);
            break;}

        case 0x28: // SRA B (Z 0 0 0)
        case 0x29: // SRA C (Z 0 0 0)
        case 0x2A: // SRA D (Z 0 0 0)
        case 0x2B: // SRA E (Z 0 0 0)
        case 0x2C: // SRA H (Z 0 0 0)
        case 0x2D: // SRA L (Z 0 0 0)
        case 0x2E: // SRA (HL) (Z 0 0 0)
        case 0x2F:{// SRA A (Z 0 0 0)
            // Shift right into Carry. High bit doesn't change.
            U8 old_low_bit = (operand & 0x01);
            U8 high_bit = (operand & 0x80);
            operand = (operand >> 1) | high_bit;
            r->set_zero_flag(operand == 0);
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            //TODO: resolve conflicting specs regarding value of carry flag here
            r->set_carry_flag(old_low_bit);
            break;}

        case 0x30: // SWAP B (Z 0 0 0)
        case 0x31: // SWAP C (Z 0 0 0)
        case 0x32: // SWAP D (Z 0 0 0)
        case 0x33: // SWAP E (Z 0 0 0)
        case 0x34: // SWAP H (Z 0 0 0)
        case 0x35: // SWAP L (Z 0 0 0)
        case 0x36: // SWAP (HL) (Z 0 0 0)
        case 0x37: // SWAP A (Z 0 0 0)
            // Swap upper and lower nibbles
            operand = (operand << 4) | (operand >> 4);
            r->set_zero_flag(operand == 0);
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            r->set_carry_flag(0);
            break;

        case 0x38: // SRL B (Z 0 0 C)
        case 0x39: // SRL C (Z 0 0 C)
        case 0x3A: // SRL D (Z 0 0 C)
        case 0x3B: // SRL E (Z 0 0 C)
        case 0x3C: // SRL H (Z 0 0 C)
        case 0x3D: // SRL L (Z 0 0 C)
        case 0x3E: // SRL (HL) (Z 0 0 C)
        case 0x3F:{// SRL A (Z 0 0 C)
            // Shift right into Carry. High bit set to 0.
            U8 old_low_bit = (operand & 0x01);
            operand >>= 1;
            r->set_zero_flag(operand == 0);
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            r->set_carry_flag(old_low_bit);
            break;}

        case 0x40: // BIT 0,B (Z 0 1 -)
        case 0x41: // BIT 0,C (Z 0 1 -)
        case 0x42: // BIT 0,D (Z 0 1 -)
        case 0x43: // BIT 0,E (Z 0 1 -)
        case 0x44: // BIT 0,H (Z 0 1 -)
        case 0x45: // BIT 0,L (Z 0 1 -)
        case 0x46: // BIT 0,(HL) (Z 0 1 -)
        case 0x47: // BIT 0,A (Z 0 1 -)

        case 0x48: // BIT 1,B (Z 0 1 -)
        case 0x49: // BIT 1,C (Z 0 1 -)
        case 0x4A: // BIT 1,D (Z 0 1 -)
        case 0x4B: // BIT 1,E (Z 0 1 -)
        case 0x4C: // BIT 1,H (Z 0 1 -)
        case 0x4D: // BIT 1,L (Z 0 1 -)
        case 0x4E: // BIT 1,(HL) (Z 0 1 -)
        case 0x4F: // BIT 1,A (Z 0 1 -)

        case 0x50: // BIT 2,B (Z 0 1 -)
        case 0x51: // BIT 2,C (Z 0 1 -)
        case 0x52: // BIT 2,D (Z 0 1 -)
        case 0x53: // BIT 2,E (Z 0 1 -)
        case 0x54: // BIT 2,H (Z 0 1 -)
        case 0x55: // BIT 2,L (Z 0 1 -)
        case 0x56: // BIT 2,(HL) (Z 0 1 -)
        case 0x57: // BIT 2,A (Z 0 1 -)

        case 0x58: // BIT 3,B (Z 0 1 -)
        case 0x59: // BIT 3,C (Z 0 1 -)
        case 0x5A: // BIT 3,D (Z 0 1 -)
        case 0x5B: // BIT 3,E (Z 0 1 -)
        case 0x5C: // BIT 3,H (Z 0 1 -)
        case 0x5D: // BIT 3,L (Z 0 1 -)
        case 0x5E: // BIT 3,(HL) (Z 0 1 -)
        case 0x5F: // BIT 3,A (Z 0 1 -)

        case 0x60: // BIT 4,B (Z 0 1 -)
        case 0x61: // BIT 4,C (Z 0 1 -)
        case 0x62: // BIT 4,D (Z 0 1 -)
        case 0x63: // BIT 4,E (Z 0 1 -)
        case 0x64: // BIT 4,H (Z 0 1 -)
        case 0x65: // BIT 4,L (Z 0 1 -)
        case 0x66: // BIT 4,(HL) (Z 0 1 -)
        case 0x67: // BIT 4,A (Z 0 1 -)

        case 0x68: // BIT 5,B (Z 0 1 -)
        case 0x69: // BIT 5,C (Z 0 1 -)
        case 0x6A: // BIT 5,D (Z 0 1 -)
        case 0x6B: // BIT 5,E (Z 0 1 -)
        case 0x6C: // BIT 5,H (Z 0 1 -)
        case 0x6D: // BIT 5,L (Z 0 1 -)
        case 0x6E: // BIT 5,(HL) (Z 0 1 -)
        case 0x6F: // BIT 5,A (Z 0 1 -)

        case 0x70: // BIT 6,B (Z 0 1 -)
        case 0x71: // BIT 6,C (Z 0 1 -)
        case 0x72: // BIT 6,D (Z 0 1 -)
        case 0x73: // BIT 6,E (Z 0 1 -)
        case 0x74: // BIT 6,H (Z 0 1 -)
        case 0x75: // BIT 6,L (Z 0 1 -)
        case 0x76: // BIT 6,(HL) (Z 0 1 -)
        case 0x77: // BIT 6,A (Z 0 1 -)

        case 0x78: // BIT 7,B (Z 0 1 -)
        case 0x79: // BIT 7,C (Z 0 1 -)
        case 0x7A: // BIT 7,D (Z 0 1 -)
        case 0x7B: // BIT 7,E (Z 0 1 -)
        case 0x7C: // BIT 7,H (Z 0 1 -)
        case 0x7D: // BIT 7,L (Z 0 1 -)
        case 0x7E: // BIT 7,(HL) (Z 0 1 -)
        case 0x7F:{// BIT 7,A (Z 0 1 -)
            U8 bit_number = (cb_instr - 0x40) / 8;
            U8 bit_mask = (0x01 << bit_number);
            r->set_zero_flag((operand & bit_mask) == 0);
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(1);
            break;}

        case 0x80: // RES 0,B (- - - -)
        case 0x81: // RES 0,C (- - - -)
        case 0x82: // RES 0,D (- - - -)
        case 0x83: // RES 0,E (- - - -)
        case 0x84: // RES 0,H (- - - -)
        case 0x85: // RES 0,L (- - - -)
        case 0x86: // RES 0,(HL) (- - - -)
        case 0x87: // RES 0,A (- - - -)

        case 0x88: // RES 1,B (- - - -)
        case 0x89: // RES 1,C (- - - -)
        case 0x8A: // RES 1,D (- - - -)
        case 0x8B: // RES 1,E (- - - -)
        case 0x8C: // RES 1,H (- - - -)
        case 0x8D: // RES 1,L (- - - -)
        case 0x8E: // RES 1,(HL) (- - - -)
        case 0x8F: // RES 1,A (- - - -)

        case 0x90: // RES 2,B (- - - -)
        case 0x91: // RES 2,C (- - - -)
        case 0x92: // RES 2,D (- - - -)
        case 0x93: // RES 2,E (- - - -)
        case 0x94: // RES 2,H (- - - -)
        case 0x95: // RES 2,L (- - - -)
        case 0x96: // RES 2,(HL) (- - - -)
        case 0x97: // RES 2,A (- - - -)

        case 0x98: // RES 3,B (- - - -)
        case 0x99: // RES 3,C (- - - -)
        case 0x9A: // RES 3,D (- - - -)
        case 0x9B: // RES 3,E (- - - -)
        case 0x9C: // RES 3,H (- - - -)
        case 0x9D: // RES 3,L (- - - -)
        case 0x9E: // RES 3,(HL) (- - - -)
        case 0x9F: // RES 3,A (- - - -)

        case 0xA0: // RES 4,B (- - - -)
        case 0xA1: // RES 4,C (- - - -)
        case 0xA2: // RES 4,D (- - - -)
        case 0xA3: // RES 4,E (- - - -)
        case 0xA4: // RES 4,H (- - - -)
        case 0xA5: // RES 4,L (- - - -)
        case 0xA6: // RES 4,(HL) (- - - -)
        case 0xA7: // RES 4,A (- - - -)

        case 0xA8: // RES 5,B (- - - -)
        case 0xA9: // RES 5,C (- - - -)
        case 0xAA: // RES 5,D (- - - -)
        case 0xAB: // RES 5,E (- - - -)
        case 0xAC: // RES 5,H (- - - -)
        case 0xAD: // RES 5,L (- - - -)
        case 0xAE: // RES 5,(HL) (- - - -)
        case 0xAF: // RES 5,A (- - - -)

        case 0xB0: // RES 6,B (- - - -)
        case 0xB1: // RES 6,C (- - - -)
        case 0xB2: // RES 6,D (- - - -)
        case 0xB3: // RES 6,E (- - - -)
        case 0xB4: // RES 6,H (- - - -)
        case 0xB5: // RES 6,L (- - - -)
        case 0xB6: // RES 6,(HL) (- - - -)
        case 0xB7: // RES 6,A (- - - -)

        case 0xB8: // RES 7,B (- - - -)
        case 0xB9: // RES 7,C (- - - -)
        case 0xBA: // RES 7,D (- - - -)
        case 0xBB: // RES 7,E (- - - -)
        case 0xBC: // RES 7,H (- - - -)
        case 0xBD: // RES 7,L (- - - -)
        case 0xBE: // RES 7,(HL) (- - - -)
        case 0xBF:{// RES 7,A (- - - -)
            U8 bit_number = (cb_instr - 0x80) / 8;
            U8 bit_mask = (0x01 << bit_number);
            operand &= ~bit_mask;
            break;}

        case 0xC0: // SET 0,B (- - - -)
        case 0xC1: // SET 0,C (- - - -)
        case 0xC2: // SET 0,D (- - - -)
        case 0xC3: // SET 0,E (- - - -)
        case 0xC4: // SET 0,H (- - - -)
        case 0xC5: // SET 0,L (- - - -)
        case 0xC6: // SET 0,(HL) (- - - -)
        case 0xC7: // SET 0,A (- - - -)

        case 0xC8: // SET 1,B (- - - -)
        case 0xC9: // SET 1,C (- - - -)
        case 0xCA: // SET 1,D (- - - -)
        case 0xCB: // SET 1,E (- - - -)
        case 0xCC: // SET 1,H (- - - -)
        case 0xCD: // SET 1,L (- - - -)
        case 0xCE: // SET 1,(HL) (- - - -)
        case 0xCF: // SET 1,A (- - - -)

        case 0xD0: // SET 2,B (- - - -)
        case 0xD1: // SET 2,C (- - - -)
        case 0xD2: // SET 2,D (- - - -)
        case 0xD3: // SET 2,E (- - - -)
        case 0xD4: // SET 2,H (- - - -)
        case 0xD5: // SET 2,L (- - - -)
        case 0xD6: // SET 2,(HL) (- - - -)
        case 0xD7: // SET 2,A (- - - -)

        case 0xD8: // SET 3,B (- - - -)
        case 0xD9: // SET 3,C (- - - -)
        case 0xDA: // SET 3,D (- - - -)
        case 0xDB: // SET 3,E (- - - -)
        case 0xDC: // SET 3,H (- - - -)
        case 0xDD: // SET 3,L (- - - -)
        case 0xDE: // SET 3,(HL) (- - - -)
        case 0xDF: // SET 3,A (- - - -)

        case 0xE0: // SET 4,B (- - - -)
        case 0xE1: // SET 4,C (- - - -)
        case 0xE2: // SET 4,D (- - - -)
        case 0xE3: // SET 4,E (- - - -)
        case 0xE4: // SET 4,H (- - - -)
        case 0xE5: // SET 4,L (- - - -)
        case 0xE6: // SET 4,(HL) (- - - -)
        case 0xE7: // SET 4,A (- - - -)

        case 0xE8: // SET 5,B (- - - -)
        case 0xE9: // SET 5,C (- - - -)
        case 0xEA: // SET 5,D (- - - -)
        case 0xEB: // SET 5,E (- - - -)
        case 0xEC: // SET 5,H (- - - -)
        case 0xED: // SET 5,L (- - - -)
        case 0xEE: // SET 5,(HL) (- - - -)
        case 0xEF: // SET 5,A (- - - -)

        case 0xF0: // SET 6,B (- - - -)
        case 0xF1: // SET 6,C (- - - -)
        case 0xF2: // SET 6,D (- - - -)
        case 0xF3: // SET 6,E (- - - -)
        case 0xF4: // SET 6,H (- - - -)
        case 0xF5: // SET 6,L (- - - -)
        case 0xF6: // SET 6,(HL) (- - - -)
        case 0xF7: // SET 6,A (- - - -)

        case 0xF8: // SET 7,B (- - - -)
        case 0xF9: // SET 7,C (- - - -)
        case 0xFA: // SET 7,D (- - - -)
        case 0xFB: // SET 7,E (- - - -)
        case 0xFC: // SET 7,H (- - - -)
        case 0xFD: // SET 7,L (- - - -)
        case 0xFE: // SET 7,(HL) (- - - -)
        case 0xFF:{// SET 7,A (- - - -)
            U8 bit_number = (cb_instr - 0xC0) / 8;
            U8 bit_mask = (0x01 << bit_number);
            operand |= bit_mask;
            break;}
    }

    emu_standard_operand_write(emu, cb_instr, operand);
}

// returns number of cycles used
U8 emu_apply_next_instruction(Emulator* emu) {
    CPU::Registers* const r = &emu->registers;

    const U8 instr[3] = {
        emu->mem_read(r->pc),
        emu->mem_read(r->pc + 1),
        emu->mem_read(r->pc + 2)
    };
    const U8 opcode = instr[0];
    const U8 direct_u8 = instr[1];
    const S8 direct_s8 = (S8)direct_u8;
    const U16 direct_u16 = *((U16*)&instr[1]);
    const CPU::OpcodeDesc& opcode_description = CPU::Opcodes[opcode];

    U8 additional_cycles = 0; //for conditional instructions

    /*
     Step to next instruction.

     NB: This _must_ happen before the instruction is actually executed. All instructions assume
     that PC is the address of the _next_ instruction.
     */
    r->pc += opcode_description.byte_length;

    switch(opcode){

        case 0x00: // NOP (- - - -)
            break;

        case 0x01: // LD BC,d16 (- - - -)
            r->bc = direct_u16;
            break;

        case 0x02:{// LD (BC),A (- - - -)
            emu->mem_write(r->bc, r->a);
            break;}

        case 0x03: // INC BC (- - - -)
            r->bc += 1;
            break;

        case 0x04: // INC B (Z 0 H -)
            inc_u8_impl(&r->b, r);
            break;

        case 0x05: // DEC B (Z 1 H -)
            dec_u8_impl(&r->b, r);
            break;

        case 0x06: // LD B,d8 (- - - -)
            r->b = direct_u8;
            break;

        case 0x07:{// RLCA (0 0 0 C)
            //TODO: resolve conlflicting specs on how to set zero flag
            r->set_zero_flag(0);
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            rlc(&r->a, r);
            break;}

        case 0x08: // LD (a16),SP (- - - -)
            emu->mem_write(direct_u16, r->sp);
            break;

        case 0x09: // ADD HL,BC (- 0 H C)
            add_hl_impl(r->bc, r);
            break;

        case 0x0A: // LD A,(BC) (- - - -)
            r->a = emu->mem_read(r->bc);
            break;

        case 0x0B: // DEC BC (- - - -)
            r->bc -= 1;
            break;

        case 0x0C: // INC C (Z 0 H -)
            inc_u8_impl(&r->c, r);
            break;

        case 0x0D: // DEC C (Z 1 H -)
            dec_u8_impl(&r->c, r);
            break;

        case 0x0E: // LD C,d8 (- - - -)
            r->c = direct_u8;
            break;

        case 0x0F:{// RRCA (0 0 0 C)
            //TODO: resolve conlflicting specs on how to set zero flag
            r->set_zero_flag(0);
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            rrc(&r->a, r);
            break;}

        case 0x10: // STOP 0 (- - - -)
            //TODO: halt CPU and LCD display until button is pressed
            break;

        case 0x11: // LD DE,d16 (- - - -)
            r->de = direct_u16;
            break;

        case 0x12: // LD (DE),A (- - - -)
            emu->mem_write(r->de, r->a);
            break;

        case 0x13: // INC DE (- - - -)
            r->de += 1;
            break;

        case 0x14: // INC D (Z 0 H -)
            inc_u8_impl(&r->d, r);
            break;

        case 0x15: // DEC D (Z 1 H -)
            dec_u8_impl(&r->d, r);
            break;

        case 0x16: // LD D,d8 (- - - -)
            r->d = direct_u8;
            break;

        case 0x17:{// RLA (0 0 0 C)
            //TODO: resolve conlflicting specs on how to set zero flag
            r->set_zero_flag(0);
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            rl(&r->a, r);
            break;}

        case 0x18: // JR r8 (- - - -)
            //TODO: check all the jumps (and maybe other opcodes) to see which should be _signed_ offsets
            r->pc += direct_s8;
            break;

        case 0x19: // ADD HL,DE (- 0 H C)
            add_hl_impl(r->de, r);
            break;

        case 0x1A: // LD A,(DE) (- - - -)
            r->a = emu->mem_read(r->de);
            break;

        case 0x1B: // DEC DE (- - - -)
            r->de -= 1;
            break;

        case 0x1C: // INC E (Z 0 H -)
            inc_u8_impl(&r->e, r);
            break;

        case 0x1D: // DEC E (Z 1 H -)
            dec_u8_impl(&r->e, r);
            break;

        case 0x1E: // LD E,d8 (- - - -)
            r->e = direct_u8;
            break;

        case 0x1F: // RRA (0 0 0 C)
            //TODO: resolve conlflicting specs on how to set zero flag
            r->set_zero_flag(0);
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            rr(&r->a, r);
            break;

        case 0x20: // JR NZ,r8 (- - - -)
            if(!r->zero_flag()){
                additional_cycles = 4;
                r->pc += direct_s8;
            }
            break;

        case 0x21: // LD HL,d16 (- - - -)
            r->hl = direct_u16;
            break;

        case 0x22: // LD (HL+),A (- - - -)
            emu->mem_write(r->hl, r->a);
            r->hl += 1;
            break;

        case 0x23: // INC HL (- - - -)
            r->hl += 1;
            break;

        case 0x24: // INC H (Z 0 H -)
            inc_u8_impl(&r->h, r);
            break;

        case 0x25: // DEC H (Z 1 H -)
            dec_u8_impl(&r->e, r);
            break;

        case 0x26: // LD H,d8 (- - - -)
            r->h = direct_u8;
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
            U8 correction = 0;

            //lower nibble
            if((r->a & 0x0F) > 0x09 || r->halfcarry_flag()){
                correction |= 0x06;
            }

            //upper nibble
            if((r->a & 0xF0) > 0x99 || r->carry_flag()){
                correction |= 0x60;
                r->set_carry_flag(1);
            } else {
                r->set_carry_flag(0);
            }

            //apply correction
            if(r->subtract_flag()){
                r->a -= correction;
            } else {
                r->a += correction;
            }

            r->set_zero_flag((r->a == 0));
            r->set_halfcarry_flag(0);
            break;}

        case 0x28: // JR Z,r8 (- - - -)
            if(r->zero_flag()){
                additional_cycles = 4;
                r->pc += direct_s8;
            }
            break;

        case 0x29: // ADD HL,HL (- 0 H C)
            add_hl_impl(r->hl, r);
            break;

        case 0x2A: // LD A,(HL+) (- - - -)
            r->a = emu->mem_read(r->hl);
            r->hl += 1;
            break;

        case 0x2B: // DEC HL (- - - -)
            r->hl -= 1;
            break;

        case 0x2C: // INC L (Z 0 H -)
            inc_u8_impl(&r->l, r);
            break;

        case 0x2D: // DEC L (Z 1 H -)
            dec_u8_impl(&r->l, r);
            break;

        case 0x2E: // LD L,d8 (- - - -)
            r->l = direct_u8;
            break;

        case 0x2F: // CPL (- 1 1 -)
            r->a = ~(r->a);
            r->set_subtract_flag(1);
            r->set_halfcarry_flag(1);
            break;

        case 0x30: // JR NC,r8 (- - - -)
            if(!r->carry_flag()){
                additional_cycles = 4;
                r->pc += direct_s8;
            }
            break;

        case 0x31: // LD SP,d16 (- - - -)
            r->sp = direct_u16;
            break;

        case 0x32: // LD (HL-),A (- - - -)
            emu->mem_write(r->hl, r->a);
            r->hl -= 1;
            break;

        case 0x33: // INC SP (- - - -)
            r->sp += 1;
            break;

        case 0x34:{// INC (HL) (Z 0 H -)
            U8 x = emu->mem_read(r->hl);
            inc_u8_impl(&x, r);
            emu->mem_write(r->hl, x);
            break;}

        case 0x35:{// DEC (HL) (Z 1 H -)
            U8 x = emu->mem_read(r->hl);
            dec_u8_impl(&x, r);
            emu->mem_write(r->hl, x);
            break;}

        case 0x36: // LD (HL),d8 (- - - -)
            emu->mem_write(r->hl, direct_u8);
            break;

        case 0x37: // SCF (- 0 0 1)
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            r->set_carry_flag(1);
            break;

        case 0x38: // JR C,r8 (- - - -)
            if(r->carry_flag()){
                additional_cycles = 4;
                r->pc += direct_s8;
            }
            break;

        case 0x39: // ADD HL,SP (- 0 H C)
            add_hl_impl(r->sp, r);
            break;

        case 0x3A: // LD A,(HL-) (- - - -)
            r->a = emu->mem_read(r->hl);
            r->hl -= 1;
            break;

        case 0x3B: // DEC SP (- - - -)
            r->sp -= 1;
            break;

        case 0x3C: // INC A (Z 0 H -)
            inc_u8_impl(&r->a, r);
            break;

        case 0x3D: // DEC A (Z 1 H -)
            dec_u8_impl(&r->a, r);
            break;

        case 0x3E: // LD A,d8 (- - - -)
            r->a = direct_u8;
            break;

        case 0x3F: // CCF (- 0 0 C)
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(0);
            r->set_carry_flag(!r->carry_flag());
            break;

        case 0x40: // LD B,B (- - - -)
        case 0x41: // LD B,C (- - - -)
        case 0x42: // LD B,D (- - - -)
        case 0x43: // LD B,E (- - - -)
        case 0x44: // LD B,H (- - - -)
        case 0x45: // LD B,L (- - - -)
        case 0x46: // LD B,(HL) (- - - -)
        case 0x47: // LD B,A (- - - -)
            r->b = emu_standard_operand_read(emu, opcode);
            break;

        case 0x48: // LD C,B (- - - -)
        case 0x49: // LD C,C (- - - -)
        case 0x4A: // LD C,D (- - - -)
        case 0x4B: // LD C,E (- - - -)
        case 0x4C: // LD C,H (- - - -)
        case 0x4D: // LD C,L (- - - -)
        case 0x4E: // LD C,(HL) (- - - -)
        case 0x4F: // LD C,A (- - - -)
            r->c = emu_standard_operand_read(emu, opcode);
            break;

        case 0x50: // LD D,B (- - - -)
        case 0x51: // LD D,C (- - - -)
        case 0x52: // LD D,D (- - - -)
        case 0x53: // LD D,E (- - - -)
        case 0x54: // LD D,H (- - - -)
        case 0x55: // LD D,L (- - - -)
        case 0x56: // LD D,(HL) (- - - -)
        case 0x57: // LD D,A (- - - -)
            r->d = emu_standard_operand_read(emu, opcode);
            break;

        case 0x58: // LD E,B (- - - -)
        case 0x59: // LD E,C (- - - -)
        case 0x5A: // LD E,D (- - - -)
        case 0x5B: // LD E,E (- - - -)
        case 0x5C: // LD E,H (- - - -)
        case 0x5D: // LD E,L (- - - -)
        case 0x5E: // LD E,(HL) (- - - -)
        case 0x5F: // LD E,A (- - - -)
            r->e = emu_standard_operand_read(emu, opcode);
            break;

        case 0x60: // LD H,B (- - - -)
        case 0x61: // LD H,C (- - - -)
        case 0x62: // LD H,D (- - - -)
        case 0x63: // LD H,E (- - - -)
        case 0x64: // LD H,H (- - - -)
        case 0x65: // LD H,L (- - - -)
        case 0x66: // LD H,(HL) (- - - -)
        case 0x67: // LD H,A (- - - -)
            r->h = emu_standard_operand_read(emu, opcode);
            break;

        case 0x68: // LD L,B (- - - -)
        case 0x69: // LD L,C (- - - -)
        case 0x6A: // LD L,D (- - - -)
        case 0x6B: // LD L,E (- - - -)
        case 0x6C: // LD L,H (- - - -)
        case 0x6D: // LD L,L (- - - -)
        case 0x6E: // LD L,(HL) (- - - -)
        case 0x6F: // LD L,A (- - - -)
            r->l = emu_standard_operand_read(emu, opcode);
            break;

        case 0x70: // LD (HL),B (- - - -)
        case 0x71: // LD (HL),C (- - - -)
        case 0x72: // LD (HL),D (- - - -)
        case 0x73: // LD (HL),E (- - - -)
        case 0x74: // LD (HL),H (- - - -)
        case 0x75: // LD (HL),L (- - - -)
            //   0x76 is the HALT opcode
        case 0x77: // LD (HL),A (- - - -)
            emu->mem_write(r->hl, emu_standard_operand_read(emu, opcode));
            break;

        case 0x76: // HALT (- - - -)
            //TODO: here
            break;

        case 0x78: // LD A,B (- - - -)
        case 0x79: // LD A,C (- - - -)
        case 0x7A: // LD A,D (- - - -)
        case 0x7B: // LD A,E (- - - -)
        case 0x7C: // LD A,H (- - - -)
        case 0x7D: // LD A,L (- - - -)
        case 0x7E: // LD A,(HL) (- - - -)
        case 0x7F: // LD A,A (- - - -)
            r->a = emu_standard_operand_read(emu, opcode);
            break;

        case 0x80: // ADD A,B (Z 0 H C)
        case 0x81: // ADD A,C (Z 0 H C)
        case 0x82: // ADD A,D (Z 0 H C)
        case 0x83: // ADD A,E (Z 0 H C)
        case 0x84: // ADD A,H (Z 0 H C)
        case 0x85: // ADD A,L (Z 0 H C)
        case 0x86: // ADD A,(HL) (Z 0 H C)
        case 0x87: // ADD A,A (Z 0 H C)

        case 0x88: // ADC A,B (Z 0 H C)
        case 0x89: // ADC A,C (Z 0 H C)
        case 0x8A: // ADC A,D (Z 0 H C)
        case 0x8B: // ADC A,E (Z 0 H C)
        case 0x8C: // ADC A,H (Z 0 H C)
        case 0x8D: // ADC A,L (Z 0 H C)
        case 0x8E: // ADC A,(HL) (Z 0 H C)
        case 0x8F:{// ADC A,A (Z 0 H C)
            add_a_impl(emu_standard_operand_read(emu, opcode), (opcode >= 0x88), r);
            break;}

        case 0x90: // SUB B (Z 1 H C)
        case 0x91: // SUB C (Z 1 H C)
        case 0x92: // SUB D (Z 1 H C)
        case 0x93: // SUB E (Z 1 H C)
        case 0x94: // SUB H (Z 1 H C)
        case 0x95: // SUB L (Z 1 H C)
        case 0x96: // SUB (HL) (Z 1 H C)
        case 0x97: // SUB A (Z 1 H C)

        case 0x98: // SBC A,B (Z 1 H C)
        case 0x99: // SBC A,C (Z 1 H C)
        case 0x9A: // SBC A,D (Z 1 H C)
        case 0x9B: // SBC A,E (Z 1 H C)
        case 0x9C: // SBC A,H (Z 1 H C)
        case 0x9D: // SBC A,L (Z 1 H C)
        case 0x9E: // SBC A,(HL) (Z 1 H C)
        case 0x9F: // SBC A,A (Z 1 H C)
            sub_a_impl(emu_standard_operand_read(emu, opcode), (opcode >= 0x98), r);
            break;

        case 0xA0: // AND B (Z 0 1 0)
        case 0xA1: // AND C (Z 0 1 0)
        case 0xA2: // AND D (Z 0 1 0)
        case 0xA3: // AND E (Z 0 1 0)
        case 0xA4: // AND H (Z 0 1 0)
        case 0xA5: // AND L (Z 0 1 0)
        case 0xA6: // AND (HL) (Z 0 1 0)
        case 0xA7: // AND A (Z 0 1 0)
            and_a_impl(emu_standard_operand_read(emu, opcode), r);
            break;

        case 0xA8: // XOR B (Z 0 0 0)
        case 0xA9: // XOR C (Z 0 0 0)
        case 0xAA: // XOR D (Z 0 0 0)
        case 0xAB: // XOR E (Z 0 0 0)
        case 0xAC: // XOR H (Z 0 0 0)
        case 0xAD: // XOR L (Z 0 0 0)
        case 0xAE: // XOR (HL) (Z 0 0 0)
        case 0xAF: // XOR A (Z 0 0 0)
            xor_a_impl(emu_standard_operand_read(emu, opcode), r);
            break;

        case 0xB0: // OR B (Z 0 0 0)
        case 0xB1: // OR C (Z 0 0 0)
        case 0xB2: // OR D (Z 0 0 0)
        case 0xB3: // OR E (Z 0 0 0)
        case 0xB4: // OR H (Z 0 0 0)
        case 0xB5: // OR L (Z 0 0 0)
        case 0xB6: // OR (HL) (Z 0 0 0)
        case 0xB7: // OR A (Z 0 0 0)
            or_a_impl(emu_standard_operand_read(emu, opcode), r);
            break;

        case 0xB8: // CP B (Z 1 H C)
        case 0xB9: // CP C (Z 1 H C)
        case 0xBA: // CP D (Z 1 H C)
        case 0xBB: // CP E (Z 1 H C)
        case 0xBC: // CP H (Z 1 H C)
        case 0xBD: // CP L (Z 1 H C)
        case 0xBE: // CP (HL) (Z 1 H C)
        case 0xBF: // CP A (Z 1 H C)
            cp_a_impl(emu_standard_operand_read(emu, opcode), r);
            break;

            //Pop two bytes from stack & jump to that address.
#define RET_IMPL \
do { \
r->pc = emu->stack_pop(); \
} while(0)

#define CONDITIONAL_RET_IMPL(CONDITION) \
do { \
if(CONDITION) { \
additional_cycles += 12; \
RET_IMPL; \
} \
} while(0)

        case 0xC0: // RET NZ (- - - -)
            CONDITIONAL_RET_IMPL(!r->zero_flag());
            break;
        case 0xC8: // RET Z (- - - -)
            CONDITIONAL_RET_IMPL(r->zero_flag());
            break;
        case 0xD0: // RET NC (- - - -)
            CONDITIONAL_RET_IMPL(!r->carry_flag());
            break;
        case 0xD8: // RET C (- - - -)
            CONDITIONAL_RET_IMPL(r->carry_flag());
            break;

        case 0xC1: // POP BC (- - - -)
            r->bc = emu->stack_pop();
            break;

        case 0xC2: // JP NZ,a16 (- - - -)
            if(!r->zero_flag()){
                additional_cycles = 4;
                r->pc = direct_u16;
            }
            break;

        case 0xC3: // JP a16 (- - - -)
            r->pc = direct_u16;
            break;

#define CALL_IMPL(ADDRESS) \
do { \
emu->stack_push(r->pc); \
r->pc = ADDRESS; \
} while(0)


#define CONDITIONAL_CALL_IMPL(COND, ADDRESS) \
do {\
if(COND){ \
additional_cycles = 12; \
CALL_IMPL(ADDRESS); \
} \
} while(0)

        case 0xC4: // CALL NZ,a16 (- - - -)
            CONDITIONAL_CALL_IMPL(!r->zero_flag(), direct_u16);
            break;
        case 0xCC: // CALL Z,a16 (- - - -)
            CONDITIONAL_CALL_IMPL(r->zero_flag(), direct_u16);
            break;
        case 0xD4: // CALL NC,a16 (- - - -)
            CONDITIONAL_CALL_IMPL(!r->carry_flag(), direct_u16);
            break;
        case 0xDC: // CALL C,a16 (- - - -)
            CONDITIONAL_CALL_IMPL(r->carry_flag(), direct_u16);
            break;

        case 0xC5: // PUSH BC (- - - -)
            emu->stack_push(r->bc);
            break;

        case 0xC6: // ADD A,d8 (Z 0 H C)
            add_a_impl(direct_u8, False, r);
            break;

        case 0xC9: // RET (- - - -)
            RET_IMPL;
            break;

        case 0xCA: // JP Z,a16 (- - - -)
            if(r->zero_flag()){
                additional_cycles = 4;
                r->pc = direct_u16;
            }
            break;

        case 0xCB:{// PREFIX CB
            U8 cb_opcode = direct_u8;
            emu_cb_instruction(emu, cb_opcode);
            additional_cycles = CPU::CBPrefixedOpcodes[cb_opcode].cycles;
            break;}

        case 0xCD: // CALL a16 (- - - -)
            CALL_IMPL(direct_u16);
            break;

        case 0xCE: // ADC A,d8 (Z 0 H C)
            add_a_impl(direct_u8, True, r);
            break;

        case 0xD1: // POP DE (- - - -)
            r->de = emu->stack_pop();
            break;

        case 0xD2: // JP NC,a16 (- - - -)
            if(!r->carry_flag()){
                additional_cycles = 4;
                r->pc = direct_u16;
            }
            break;

        case 0xD5: // PUSH DE (- - - -)
            emu->stack_push(r->de);
            break;

        case 0xD6: // SUB d8 (Z 1 H C)
            sub_a_impl(direct_u8, False, r);
            break;

        case 0xD9: // RETI (- - - -)
            RET_IMPL;
            //TODO: enable interrupts here
            break;

        case 0xDA: // JP C,a16 (- - - -)
            if(r->carry_flag()){
                additional_cycles = 4;
                r->pc = direct_u16;
            }
            break;

        case 0xDE: // SBC A,d8 (Z 1 H C)
            sub_a_impl(direct_u8, True, r);
            break;

        case 0xE0: // LDH (a8),A (- - - -)
            emu->mem_write(0xFF00 + (U16)direct_u8, r->a);
            break;

        case 0xE1: // POP HL (- - - -)
            r->hl = emu->stack_pop();
            break;

        case 0xE2: // LD (C),A (- - - -)
            emu->mem_write(0xFF00 + (U16)r->c, r->a);
            break;


        case 0xE5: // PUSH HL (- - - -)
            emu->stack_push(r->hl);
            break;

        case 0xE6: // AND d8 (Z 0 1 0)
            and_a_impl(direct_u8, r);
            break;

        case 0xE8:{// ADD SP,r8 (0 0 H C)
            //TODO: the operand should be _signed_, but how does that affect the carry flags?
            U16 operand = direct_u8;
            r->set_zero_flag(0);
            r->set_subtract_flag(0);
            r->set_halfcarry_flag(u16_add_will_halfcarry(r->sp, operand));
            r->set_carry_flag(u16_add_will_carry(r->sp, operand));
            r->sp += operand;
            break;}

        case 0xE9: // JP (HL) (- - - -)
            r->pc = emu->mem_read(r->hl);
            break;

        case 0xEA: // LD (a16),A (- - - -)
            emu->mem_write(direct_u16, r->a);
            break;

        case 0xEE: // XOR d8 (Z 0 0 0)
            xor_a_impl(direct_u8, r);
            break;

        case 0xF0: // LDH A,(a8) (- - - -)
            r->a = emu->mem_read(0xFF00 + (U16)direct_u8);
            break;

        case 0xF1: // POP AF (Z N H C)
            // all flags set by virtue of setting r->f
            r->af = emu->stack_pop();
            break;

        case 0xF2: // LD A,(C)
            r->a = emu->mem_read(0xFF00 + (U16)r->c);
            break;

        case 0xF3: // DI (- - - -)
            /*
             TODO: here

             This instruction disables interrupts but not immediately.
             Interrupts are disabled after instruction after DI
             is executed.
             */
            break;

        case 0xF5: // PUSH AF (- - - -)
            emu->stack_push(r->af);
            break;

        case 0xF6: // OR d8 (Z 0 0 0)
            or_a_impl(direct_u8, r);
            break;

        case 0xF8:{// LD HL,SP+r8 (0 0 H C)
            U16 relative_address = direct_u8;
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
            r->a = emu->mem_read(direct_u16);
            break;
            
        case 0xFB: // EI
            /*
             TODO: here
             
             Enable interrupts. This intruction enables interrupts
             but not immediately. Interrupts are enabled after instruction after EI
             is executed.
             */
            break;
            
        case 0xFE: // CP d8 (Z 1 H C)
            cp_a_impl(direct_u8, r);
            break;
            
        case 0xD3: // INVALID_INSTRUCTION
        case 0xDB: // INVALID_INSTRUCTION
        case 0xDD: // INVALID_INSTRUCTION
        case 0xE3: // INVALID_INSTRUCTION
        case 0xE4: // INVALID_INSTRUCTION
        case 0xEB: // INVALID_INSTRUCTION
        case 0xEC: // INVALID_INSTRUCTION
        case 0xED: // INVALID_INSTRUCTION
        case 0xF4: // INVALID_INSTRUCTION
        case 0xFC: // INVALID_INSTRUCTION
        case 0xFD: // INVALID_INSTRUCTION
            break;
            
        case 0xC7: // RST 0x00 (- - - -)
        case 0xCF: // RST 0x08 (- - - -)
        case 0xD7: // RST 0x10 (- - - -)
        case 0xDF: // RST 0x18 (- - - -)
        case 0xE7: // RST 0x20 (- - - -)
        case 0xEF: // RST 0x28 (- - - -)
        case 0xF7: // RST 0x30 (- - - -)
        case 0xFF: // RST 0x38 (- - - -)
            emu->stack_push(r->pc);
            r->pc = opcode - 0xC7;
            break;
    }
    
    return opcode_description.cycles + additional_cycles;
}

void randset(void* dest, size_t size) {
    for(size_t i = 0; i < size; ++i){
        ((U8*)dest)[i] = rand() % 0xFF;
    }
}

void emulator_init(Emulator* emu) {
    memset(emu->cartridge_ram, 0, sizeof(emu->cartridge_ram));
    memset(emu->internal_ram, 0, sizeof(emu->internal_ram));
    memset(&emu->hardware_registers, 0, sizeof(emu->hardware_registers));
    memset(emu->zero_page, 0, sizeof(emu->zero_page));
    memset(&emu->oam, 0, sizeof(emu->oam));
    memset(&emu->registers, 0, sizeof(emu->registers));
    memset(&emu->cart, 0, sizeof(emu->cart));
    emu->vram_mutated = False;

    // put random garbage in vram
    randset(&emu->gpu.vram, sizeof(emu->gpu.vram));
}

void emulator_step(Emulator* emu) {
    U8 cycles = emu_apply_next_instruction(emu);
    cycles += 1; //TODO: why is this += 1? is this right?

    emu->gpu.step(cycles);

    //TODO: update total cycles elapsed in emulator
}



U8 get_hardware_register(Emulator* emu, U16 address) {
    HardwareRegisters::Registers* hwr = &emu->hardware_registers;

#   define HW_REG_GET(REG_CAPS, REG_SMALL) \
        case HardwareRegisters::REG_CAPS: \
            return hwr->REG_SMALL;

    switch(address){
        HW_REG_GET(DIV, div)
        HW_REG_GET(TIMA, tima)
        HW_REG_GET(TMA, tma)
        HW_REG_GET(TAC, tac)
        HW_REG_GET(NR10, nr10)
        HW_REG_GET(NR11, nr11)
        HW_REG_GET(NR12, nr12)
        HW_REG_GET(NR13, nr13)
        HW_REG_GET(NR14, nr14)
        HW_REG_GET(NR21, nr21)
        HW_REG_GET(NR22, nr22)
        HW_REG_GET(NR23, nr23)
        HW_REG_GET(NR24, nr24)
        HW_REG_GET(NR30, nr30)
        HW_REG_GET(NR31, nr31)
        HW_REG_GET(NR32, nr32)
        HW_REG_GET(NR33, nr33)
        HW_REG_GET(NR34, nr34)
        HW_REG_GET(NR41, nr41)
        HW_REG_GET(NR42, nr42)
        HW_REG_GET(NR43, nr43)
        HW_REG_GET(NR44, nr44)
        HW_REG_GET(NR50, nr50)
        HW_REG_GET(NR51, nr51)
        HW_REG_GET(NR52, nr52)
        HW_REG_GET(LCDC, lcdc)
        HW_REG_GET(STAT, stat)
        HW_REG_GET(SCY, scy)
        HW_REG_GET(SCX, scx)

        case HardwareRegisters::LY:
            return emu->gpu.line;

        HW_REG_GET(LYC, lyc)
        HW_REG_GET(DMA, dma)
        HW_REG_GET(BGP, bgp)
        HW_REG_GET(OBP0, obp0)
        HW_REG_GET(OBP1, obp1)
        HW_REG_GET(WY, wy)
        HW_REG_GET(WX, wx)
        HW_REG_GET(P1, p1)
        HW_REG_GET(SB, sb)
        HW_REG_GET(SC, sc)
        HW_REG_GET(IF, if_)
        HW_REG_GET(BootstrapROM, bootstrap_rom)
        HW_REG_GET(IE, ie)

        default:
            if(HardwareRegisters::WavePatternStart <= address && address <= HardwareRegisters::WavePatternEnd){
                return hwr->wave_pattern[address - HardwareRegisters::WavePatternStart];
            } else {
                assert(0); //dunno this register
                return 0xFF;
            }
    }
#   undef HW_REG_GET
}

void set_hardware_register(HardwareRegisters::Registers* hwr, U16 address, U8 value) {
#   define HW_REG_SET(REG_CAPS, REG_SMALL) \
        case HardwareRegisters::REG_CAPS: \
            hwr->REG_SMALL = value; \
            return;

#   define HW_REG_ZERO(REG_CAPS, REG_SMALL) \
        case HardwareRegisters::REG_CAPS: \
            hwr->REG_SMALL = 0; \
            return;

    switch(address){
        HW_REG_ZERO(DIV, div)
        HW_REG_SET(TIMA, tima)
        HW_REG_SET(TMA, tma)
        HW_REG_SET(TAC, tac)
        HW_REG_SET(NR10, nr10)
        HW_REG_SET(NR11, nr11)
        HW_REG_SET(NR12, nr12)
        HW_REG_SET(NR13, nr13)
        HW_REG_SET(NR14, nr14)
        HW_REG_SET(NR21, nr21)
        HW_REG_SET(NR22, nr22)
        HW_REG_SET(NR23, nr23)
        HW_REG_SET(NR24, nr24)
        HW_REG_SET(NR30, nr30)
        HW_REG_SET(NR31, nr31)
        HW_REG_SET(NR32, nr32)
        HW_REG_SET(NR33, nr33)
        HW_REG_SET(NR34, nr34)
        HW_REG_SET(NR41, nr41)
        HW_REG_SET(NR42, nr42)
        HW_REG_SET(NR43, nr43)
        HW_REG_SET(NR44, nr44)
        HW_REG_SET(NR50, nr50)
        HW_REG_SET(NR51, nr51)
        HW_REG_SET(NR52, nr52)
        HW_REG_SET(LCDC, lcdc)
        HW_REG_SET(STAT, stat)
        HW_REG_SET(SCY, scy)
        HW_REG_SET(SCX, scx)

        case HardwareRegisters::LY:
            //TODO: is this writable? source says it's read only, but also says it zeros on write
            //treating as read-only for now.
            return;

        HW_REG_SET(LYC, lyc)
        HW_REG_SET(DMA, dma)
        HW_REG_SET(BGP, bgp)
        HW_REG_SET(OBP0, obp0)
        HW_REG_SET(OBP1, obp1)
        HW_REG_SET(WY, wy)
        HW_REG_SET(WX, wx)
        HW_REG_SET(P1, p1)
        HW_REG_SET(SB, sb)
        HW_REG_SET(SC, sc)
        HW_REG_SET(IF, if_)
        HW_REG_SET(BootstrapROM, bootstrap_rom)
        HW_REG_SET(IE, ie)

        default:
            if(HardwareRegisters::WavePatternStart <= address && address <= HardwareRegisters::WavePatternEnd){
                hwr->wave_pattern[address - HardwareRegisters::WavePatternStart] = value;
            } else {
                assert(0); //dunno this register
            }
    }
}

U8 Emulator::mem_read(U16 address) {
    // 0x0000 - 0x3FFF: bank 0 of cart ROM (not switchable)
    if(address <= 0x3FFF) {
        if(address <= 0x00FF && hardware_registers.bootstrap_rom == BootstrapRom_Enabled) {
            // bootstrap ROM occupies 0x0000 - 0x00FF, but only if the hardware flag is set
            return BootstrapRom[address];
        } else {
            return cart.data[address];
        }
    }

    // 0x4000 - 0x7FFF: switchable cart ROM banks
    else if(address <= 0x7FFF){
        //TODO: implement bank switching
        return cart.data[address];
    }

    // 0x8000 - 0x9FFF: video RAM
    else if(address <= 0x9FFF){
        return gpu.vram.memory[address - 0x8000];
    }

    // 0xA000 - 0xBFFF: cartrige RAM
    else if(address <= 0xBFFF) {
        return cartridge_ram[address - 0xA000];
    }

    // 0xC000 - 0xDFFF: Internal RAM
    // 0xE000 - 0xFDFF: Echo of internal RAM
    else if(address <= 0xFDFF) {
        return internal_ram[address & 0x1FFF];
    }

    // 0xFE00 - 0xFE9F: OAM - Object/Sprite Attribute Memory
    else if(address <= 0xFE9F) {
        return oam.memory[address - 0xFE00];
    }

    // 0xFEA0 - 0xFEFF: Unusable Memory
    else if(address <= 0xFEFF) {
        return 0xFF; //TODO: what happens when you read unusable memory?
    }

    // 0xFF00 - 0xFF7F: Hardware I/O Registers
    // 0xFFFF: Interrupt Enable Flag
    else if(address <= 0xFF7F || address == 0xFFFF) {
        return get_hardware_register(this, address);
    }

    // 0xFF80 - 0xFFFE: Zero Page
    else if(address <= 0xFFFE) {
        return zero_page[address - 0xFF80];
    }

    assert(0); //should never get here. All addresses should be covered
}

void Emulator::mem_write(U16 address, U8 value) {
    // 0x0000 - 0x3FFF: bank 0 of cart ROM (not switchable)
    // 0x4000 - 0x7FFF: switchable cart ROM banks
    if(address <= 0x7FFF){
        //TODO: writing into this area does memory bank switching
        assert(0);
        return;
    }

    // 0x8000 - 0x9FFF: video RAM
    else if(address <= 0x9FFF){
        vram_mutated = True;
        gpu.vram.memory[address - 0x8000] = value;
        return;
    }

    // 0xA000 - 0xBFFF: cartrige RAM
    else if(address <= 0xBFFF) {
        cartridge_ram[address - 0xA000] = value;
        return;
    }

    // 0xC000 - 0xDFFF: Internal RAM
    // 0xE000 - 0xFDFF: Echo of internal RAM
    else if(address <= 0xFDFF) {
        internal_ram[address & 0x1FFF] = value;
        return;
    }

    // 0xFE00 - 0xFE9F: OAM - Object Attribute Memory
    else if(address <= 0xFE9F) {
        vram_mutated = True;
        oam.memory[address - 0xFE00] = value;
        return;
    }

    // 0xFEA0 - 0xFEFF: Unusable Memory
    else if(address <= 0xFEFF) {
        assert(0); //TODO: what happens when writing to unusable memory?
        return;
    }

    // 0xFF00 - 0xFF7F: Hardware I/O Registers
    // 0xFFFF: Interrupt Enable Flag
    else if(address <= 0xFF7F || address == 0xFFFF) {
        set_hardware_register(&hardware_registers, address, value);
        return;
    }

    // 0xFF80 - 0xFFFE: Zero Page
    else if(address <= 0xFFFE) {
        zero_page[address - 0xFF80] = value;
        return;
    }

    assert(0); //should never get here. All addresses should be covered
}

U16 Emulator::mem_read_16(U16 address) {
    U16 value = 0;
    U8* bytes = (U8*)&value;
    bytes[0] = mem_read(address);
    bytes[1] = mem_read(address + 1);
    return value;
}

void Emulator::mem_write_16(U16 address, U16 value) {
    U8* bytes = (U8*)&value;
    mem_write(address, bytes[0]);
    mem_write(address+1, bytes[1]);
}

U16 Emulator::stack_pop() {
    U16 retval = mem_read_16(registers.sp);
    registers.sp += 2;
    return retval;
}

void Emulator::stack_push(U16 value) {
    registers.sp -= 2;
    mem_write_16(registers.sp, value);
}
