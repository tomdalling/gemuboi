#pragma once

namespace Timer {
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

    /*
     DIV - Divider Register (R/W)

     This register is incremented at rate of 16384Hz. Writing any value to this register resets it
     to 0x00.
     */
    const U16 DIV = 0xFF04;

    /*
     TIMA - Timer counter (R/W)

     This timer is incremented by a clock frequency specified by the TAC register ($FF07). 
     When the value overflows (gets bigger than 0xFF) then it will be reset to the value specified
     in TMA (FF06), and an interrupt will be requested.
     */
    const U16 TIMA = 0xFF05;

    /*
     TMA - Timer Modulo (R/W)

     When the TIMA overflows, this data will be loaded.
     */
    const U16 TMA = 0xFF06; // TMA

    /*
     TAC - Timer Control (R/W)

     Bit 2 - Timer Stop
        0: Stop Timer
        1: Start Timer
     Bits 1+0 - Input Clock Select
        00:   4.096 KHz
        01: 262.144 KHz
        10:  65.536 KHz
        11:  16.384 KHz
     */
    const U16 TAC = 0xFF07; //TAC

    const U16 OverflowInterruptAddress = 0x0050;
};