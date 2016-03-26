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
#include "hardware_registers.hpp"
#include "video.hpp"

struct Emulator {
    U8 cartridge_ram[0x2000];
    U8 internal_ram[0x2000];
    HardwareRegisters::Registers hardware_registers;
    U8 zero_page[128];
    Video::OAM oam;
    Video::GPU gpu;

    CPU::Registers registers;
    Cart::Cart cart;
    BOOL32 vram_mutated;

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
};

void emulator_init(Emulator* emu);
void emulator_step(Emulator* emu);
