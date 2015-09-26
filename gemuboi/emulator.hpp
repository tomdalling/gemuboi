//
//  emulator.hpp
//  gemuboi
//
//  Created by Tom on 24/09/2015.
//
//

#pragma once

#include "cpu.hpp"
#include "cart.hpp"

/*
 When the Gameboy is turned on, the bootstrap ROM is situated in a memory
 page at positions $0-$FF (0-255). The CPU enters at $0 at startup, and the
 last two instructions of the code writes to a special register which disables
 the internal ROM page, thus making the lower 256 bytes of the cartridge ROM
 readable. The last instruction is situated at position $FE and is two bytes big,
 which means that right after that instruction has finished, the CPU executes the
 instruction at $100, which is the entry point code on a cartridge.

 The final two instructions are:

 00FC: LD A,$01
	00FE: LD ($FF00+$50),A

 So writing 0x01 to the address 0xFF50 disables the bootstrap rom, enabling the
 address range 0x0000-0x00FF to be read from the cartridge instead.
 */
const U16 BootstrapRomFlagAddress = 0xFF50;
const U16 BootstrapRomSize = 256;
const U16 BootstrapRomFlag_Enabled = 0x00; // BS ROM is readable
//const U16 BootstrapRomFlag_Disabled = 0x01; // BS ROM not readable (replaced by cartridge ROM)
extern U8 BootstrapRom[BootstrapRomSize];

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

    U8 mem_read(U16 address);
    void mem_write(U16 address, U8 value);
    U16 mem_read_16(U16 address);
    void mem_write_16(U16 address, U16 value);
    U16 stack_pop();
    void stack_push(U16 value);
    BOOL32 bootstrap_rom_enabled();
};

void emulator_step(Emulator* emu);
void emulator_init(Emulator* emu);
