#pragma once

namespace HardwareRegisters {
    /*
     P1 (R/W)
     Register for reading joy pad info and determining system type.

         Bit 7 - Not used
         Bit 6 - Not used
         Bit 5 - P15 out port
         Bit 4 - P14 out port
         Bit 3 - P13 in port
         Bit 2 - P12 in port
         Bit 1 - P11 in port
         Bit 0 - P10 in port

     This is the matrix layout:
     
            ￼       P14        P15
                    |          |
         P10 -------O-Right----O-A
                    |          |
         P11 -------O-Left-----O-B
                    |          |
         P12 -------O-Up-------O-Select
                    |          |
         P13 -------O-Down-----O-Start
                    |          |
     */
    static const U16 P1 = 0xFF00;

    /*
     SB (R/W)
     Serial transfer data. 8 Bits of data to be read/written.
     */
    static const U16 SB = 0xFF01;

    static const U16 SC = 0xFF02;

    /*
     DIV - Divider Register (R/W)

     This register is incremented at rate of 16384Hz. Writing any value to this register resets it
     to 0x00.
     */
    static const U16 DIV = 0xFF04;

    /*
     TIMA - Timer counter (R/W)

     This timer is incremented by a clock frequency specified by the TAC register ($FF07). 
     When the value overflows (gets bigger than 0xFF) then it will be reset to the value specified
     in TMA (FF06), and an interrupt will be requested.
     */
    static const U16 TIMA = 0xFF05;

    /*
     TMA - Timer Modulo (R/W)

     When the TIMA overflows, this data will be loaded.
     */
    static const U16 TMA = 0xFF06;

    /*
     TAC - Timer Control (R/W)

     Bit 2 - Running
        0: Stop Timer
        1: Start Timer
     Bits 1+0 - Frequency
        00:   4096 Hz
        01: 262144 Hz
        10:  65536 Hz
        11:  16384 Hz
     */
    static const U16 TAC = 0xFF07;

    /*
     IF - Interrupt Flag (R/W)

        Bit 4: Transition from High to Low of Pin number P10-P13
        Bit 3: Serial I/O transfer complete
        Bit 2: Timer Overflow
        Bit 1: LCDC (see STAT)
        Bit 0: V-Blank

     The priority and jump address for the above 5 interrupts are:

        Interrupt          Priority    Address
        --------------------------------------
        V-Blank                1        0x0040
        LCDC Status            2        0x0048 - Modes 0,1,2. LYC=LY coincide. (selectable)
        Timer Overflow         3        0x0050
        Serial Transfer        4        0x0058 - when transfer is complete
        Hi-Lo of P10-P13       5        0x0060
     
     When more than 1 interrupts occur at the same time only the interrupt with the highest
     priority can be acknowledged. When an interrupt is used a '0' should be stored in the IF
     register before the IE register is set.
     */
    static const U16 IF = 0xFF0F;

    static const U16 NR10 = 0xFF10;
    static const U16 NR11 = 0xFF11;
    static const U16 NR12 = 0xFF12;
    static const U16 NR13 = 0xFF13;
    static const U16 NR14 = 0xFF14;
    static const U16 NR21 = 0xFF16;
    static const U16 NR22 = 0xFF17;
    static const U16 NR23 = 0xFF18;
    static const U16 NR24 = 0xFF19;
    static const U16 NR30 = 0xFF1A;
    static const U16 NR31 = 0xFF1B;
    static const U16 NR32 = 0xFF1C;
    static const U16 NR33 = 0xFF1D;
    static const U16 NR34 = 0xFF1E;
    static const U16 NR41 = 0xFF20;
    static const U16 NR42 = 0xFF21;
    static const U16 NR43 = 0xFF22;
    static const U16 NR44 = 0xFF23;
    static const U16 NR50 = 0xFF24;
    static const U16 NR51 = 0xFF25;
    static const U16 NR52 = 0xFF26;
    static const U16 WavePatternStart = 0xFF30;
    static const U16 WavePatternEnd = 0xFF3F;

    /*
     LCDC - LCD Control (R/W)

     Bit 7 - LCD Control Operation ^
         0: Stop completely (no picture on screen)
         1: operation
     Bit 6 - Window Tile Map Display Select
        0: $9800-$9BFF
        1: $9C00-$9FFF
     Bit 5 - Window Display
        0: off
        1: on
     Bit 4 - BG & Window Tile Data Select
        0: $8800-$97FF
        1: $8000-$8FFF <- Same area as OBJ
     Bit 3 - BG Tile Map Display Select
        0: $9800-$9BFF
        1: $9C00-$9FFF
     Bit 2 - OBJ (Sprite) Size
        0: 8*8
        1: 8*16 (width*height)
     Bit 1 - OBJ (Sprite) Display
        0: off
        1: on
     Bit 0 - BG & Window Display
        0: off
        1: on
     
     Stopping LCD operation (bit 7 from 1 to 0) must be performed during V-blank to work properly.
     V- blank can be confirmed when the value of LY is greater than or equal to 144.

     (value 0x91 at reset)
     */
    static const U16 LCDC = 0xFF40;

    /*
     STAT - LCDC Status (R/W)

     Bits 6-3 - Interrupt Selection By LCDC Status
        Bit 6 - LYC=LY Coincidence (Selectable)
        Bit 5 - Mode 10
        Bit 4 - Mode 01
        Bit 3 - Mode 00
            0: Non Selection
            1: Selection
        Bit 2 - Coincidence Flag
            0: LYC not equal to LCDC LY 1: LYC = LCDC LY
        ￼Bit 1-0 - Mode Flag
            00: During H-Blank
            01: During V-Blank
            10: During Searching OAM-RAM
            11: During Transfering Data to LCD Driver

     STAT shows the current status of the LCD controller.

     When the flag is 00 it is the H-Blank period and the CPU can access the display RAM ($8000-$9FFF).

     When the flag is 01 it is the V-Blank period and the CPU can access the display RAM ($8000-$9FFF).

     When the flag is 10 then the OAM is being used ($FE00-$FE9F). The CPU cannot access the OAM
     during this period.

     When the flag is 11 both the OAM and display RAM are being used. The CPU cannot access either
     during this period.

     The following are typical when the display is enabled:
     ￼
     Mode 0: 000___000___000___000___000___000___000________________
     Mode 1: _______________________________________11111111111111__
     Mode 2: ___2_____2_____2_____2_____2_____2___________________2_
     Mode 3: ____33____33____33____33____33____33__________________3

     The Mode Flag goes through the values 0, 2, and 3 at a cycle of about 109uS. 
     0 is present about 48.6uS, 2 about 19uS, and 3 about 41uS. This is interrupted every
     16.6ms by the VBlank (1). The mode flag stays set at 1 for about 1.08 ms.
     (Mode 0 is present between 201-207 clks, 2 about 77-83 clks, and 3 about 169-175 clks. 
     A complete cycle through these states takes 456 clks. VBlank lasts 4560 clks. A complete
     screen refresh occurs every 70224 clks.)
     */
    static const U16 STAT = 0xFF41;

    /* 
     SCY/SCX - Scroll Y/X (R/W)
     
     8 Bit value $00-$FF to scroll BG screen position.
     */
    static const U16 SCY = 0xFF42;
    static const U16 SCX = 0xFF43;

    /*
     LY - LCDC Y-Coordinate (R)

     The LY indicates the vertical line to which the present data is transferred to the LCD Driver.
     The LY can take on any value between 0 through 153. The values between 144 and 153 indicate
     the V-Blank period. Writing will reset the counter.
     */
    static const U16 LY = 0xFF44;

    /*
     LYC - LY Compare (R/W)

     The LYC compares itself with the LY. If the values are the same it causes the STAT to set the
     coincident flag.
     */
    static const U16 LYC = 0xFF45;

    /*
     DMA - DMA Transfer and Start Address (W)

     The DMA Transfer (40*28 bit) from internal ROM or RAM ($0000-$F19F) to the 
     OAM (address $FE00-$FE9F) can be performed. It takes 160 microseconds for the transfer.

     40*28 bit = #140 or #$8C. As you can see, it only transfers $8C bytes of data. 
     OAM data is $A0 bytes long, from $0-$9F.

     But if you examine the OAM data you see that 4 bits are not in use.

     40*32 bit = #$A0, but since 4 bits for each OAM is not used it's 40*28 bit.

     It transfers all the OAM data to OAM RAM.

     The DMA transfer start address can be designated every $100 from address $0000-$F100.
     That means $0000, $0100, $0200, $0300....

     As can be seen by looking at register $FF41 Sprite RAM ($FE00 - $FE9F) is not always available.
     A simple routine that many games use to write data to Sprite memory is shown below.
     Since it copies data to the sprite RAM at the appropriate times it removes that responsibility
     from the main program.

     All of the memory space, except high RAM ($FF80-$FFFE), is not accessible during DMA. 
     Because of this, the routine below must be copied & executed in high ram. It is usually called
     from a V-blank Interrupt.

     Example program:

            org $40
            jp VBlank
            org $ff80
        VBlank:
            push af         <- Save A reg & flags
            ld a,BASE_ADRS  <- transfer data from BASE_ADRS
            ld ($ff46),a    <- put A into DMA registers
            ld a,28h        <- loop length
        Wait:               <- We need to wait 160 ms.
            deca            <- 4 cycles - decrease A by 1
            jr nz,Wait      <- 12 cycles - branch if Not Zero to Wait
            pop af          <- Restore A reg & flags
            reti            <- Return from interrupt
     */
    static const U16 DMA = 0xFF46;

    /*
     BGP - BG & Window Palette Data (R/W)

     Bit 7-6 = Data for Dot Data 11 (Normally darkest color)
     Bit 5-4 = Data for Dot Data 10
     Bit 3-2 = Data for Dot Data 01
     Bit 1-0 = Data for Dot Data 00 (Normally lightest color)

     This selects the shade of grays to use for the background (BG) & window pixels. 
     Since each pixel uses 2 bits, the corresponding shade will be selected from here.
     */
    static const U16 BGP = 0xFF47;

    /*
     OBP0 - Object Palette 0 Data (R/W)

     This selects the colors for sprite palette 0. It works exactly as BGP ($FF47) except
     each value of 0 is transparent.
     */
    const U16 OBP0 = 0xFF48;

    /*
     OBP1 - Object Palette 1 Data (R/W)

     This Selects the colors for sprite palette 1. It works exactly as OBP0 ($FF48).
     See BGP for details.
     */
    const U16 OBP1 = 0xFF49;

    /*
     WY/WX - Window Y/X Position (R/W)

     0 <= WY <= 143
     0 <= WX <= 166
     
     WY must be greater than or equal to 0 and must be less than or equal to 143 for
     window to be visible.

     WX must be greater than or equal to 0 and must be less than or equal to 166 for 
     window to be visible.

     WX is offset from absolute screen coordinates by 7. Setting the window to WX=7, WY=0
     will put the upper left corner of the window at absolute screen coordinates 0,0.

     Lets say WY = 70 and WX = 87. The window would be positioned as so:


            0                  80               159
            ______________________________________
         0 |                                      |
           |                   |                  |
           |                                      |
           |            Background Display        |
           |                   Here               |
           |                                      |
           |                                      |
        70 |         -         +------------------|
           |                   | 80,70            |
           |                   |                  |
           |                   |  Window Display  |
           |                   |       Here       |
           |                   |                  |
           |                   |                  |
       143 |___________________|__________________|


     OBJ Characters (Sprites) can still enter the window. None of the window colors are transparent
     so any background tiles under the window are hidden.
     */
    static const U16 WY = 0xFF4A;
    static const U16 WX = 0xFF4B;

    /*
     When the Gameboy is turned on, the bootstrap ROM is situated in a memory
     page at positions $0-$FF (0-255). The CPU enters at $0 at startup, and the
     last two instructions of the code writes to a special register which disables
     the internal ROM page, thus making the lower 256 bytes of the cartridge ROM
     readable.

     The final two instructions are:

        00FC: LD A,$01
        00FE: LD ($FF00+$50),A

     So writing 0x01 to the address 0xFF50 disables the bootstrap rom, enabling the
     address range 0x0000-0x00FF to be read from the cartridge instead.
     */
    static const U16 BootstrapROM = 0xFF50;


    /*
     IE - Interrupt Enable (R/W)

     Bit 4: Transition from High to Low of Pin number P10-P13.
     Bit 3: Serial I/O transfer complete
     Bit 2: Timer Overflow
     Bit 1: LCDC (see STAT)
     Bit 0: V-Blank

     0: disable
     1: enable
     */
    static const U16 IE = 0xFFFF;

    struct Registers {
        // timer
        U8 div;
        U8 tima;
        U8 tma;
        U8 tac;

        // audio
        U8 nr10;
        U8 nr11;
        U8 nr12;
        U8 nr13;
        U8 nr14;
        U8 nr21;
        U8 nr22;
        U8 nr23;
        U8 nr24;
        U8 nr30;
        U8 nr31;
        U8 nr32;
        U8 nr33;
        U8 nr34;
        U8 nr41;
        U8 nr42;
        U8 nr43;
        U8 nr44;
        U8 nr50;
        U8 nr51;
        U8 nr52;
        U8 wave_pattern[16];

        // video
        U8 lcdc;
        U8 stat;
        U8 scy;
        U8 scx;
        U8 ly;
        U8 lyc;
        U8 dma;
        U8 bgp;
        U8 obp0;
        U8 obp1;
        U8 wy;
        U8 wx;

        // misc
        U8 p1;
        U8 sb;
        U8 sc;
        U8 if_;
        U8 bootstrap_rom;
        U8 ie;
    };

}; // namespace HardwareRegisters
