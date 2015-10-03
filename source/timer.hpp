#pragma once

#include "types.hpp"

/*
 The timer in the GameBoy has a selectable frequency of 4096, 16384, 65536, or 262144 Hertz.
 This frequency increments the Timer Counter (TIMA). When it overflows, it generates an interrupt.
 It is then loaded with the contents of Timer Modulo (TMA). The following are examples:

 ;This interval timer interrupts 4096 times per second
    ld a,-1
    ld ($FF06),a ;Set TMA to divide clock by 1 ld a,4
    ld ($FF07),a ;Set clock to 4096 Hertz

 ;This interval timer interrupts 65536 times per second
    ￼￼￼ld  a,-4
    ld  ($FF06),a ;Set TMA to divide clock by 4
    ld  a,5
    ld  ($FF07),a ;Set clock to 262144 Hertz
 */
namespace Timer {
    const U32 CPUClockSpeed = 4194304; //Hz


    const U8 TACRunningMask = 0x04; // Bit 2
    const U8 TACFrequencyMask = 0x03; // Bits 0 and 1
    const U8 TACFrequency4096 = 0x00;
    const U8 TACFrequency262144 = 0x01;
    const U8 TACFrequency65536 = 0x02;
    const U8 TACFrequency16384 = 0x03;
    const U16 CyclesPerTickByFrequency[4] = {
        CPUClockSpeed/4096,
        CPUClockSpeed/262144,
        CPUClockSpeed/65536,
        CPUClockSpeed/16384
    };

    /*
     The address of the interupt to call when TIMA overflows.
     */
    const U16 OverflowInterruptAddress = 0x0050;

    struct Timer {
        U8 div;
        U8 tima;
        U8 tma;
        U8 tac;

        U16 div_cycles_elapsed;
        U16 tima_cycles_elapsed;
    };
};