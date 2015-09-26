//
//  timer.cpp
//  gemuboi
//
//  Created by Tom on 26/09/2015.
//
//

#include "timer.hpp"
/*
void Timer::Update(U8 cycles_elapsed) {
    if(!(tac & TACRunningMask))
        return False;

    // update div
    div_cycles_elapsed += cycles_elapsed;
    if(div_cycles_elapsed > CyclesPerTickByFrequency[TACFrequency16384]){
        div += 1;
        div_cycles_elapsed -= CyclesPerTickByFrequency[TACFrequency16384];
    }

    // update tima
    tima_cycles_elapsed += cycles_elapsed;
    U16 tima_cycles_per_tick = CyclesPerTickByFrequency[tac & TACFrequencyMask];
    BOOL32 trigger_interrupt = False;

    if(tima_cycles_elapsed > tima_cycles_per_tick){
        tima_cycles_elapsed -= tima_cycles_per_tick;
        tima += 1;
        if(tima == 0){
            tima = tma;
            trigger_interrupt = True;
        }
    }

    return trigger_interrupt;
}*/