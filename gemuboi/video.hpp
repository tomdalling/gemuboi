# pragma once

#include "types.hpp"

namespace Video {
    /* 
     The main GameBoy screen buffer (background) consists of 256x256 pixels
     or 32x32 tiles (8x8 pixels each).
     */
    const U8 ScreenBufferWidth = 256;
    const U8 ScreenBufferHeight = 256;

    /*
     The gameboy contains two 32x32 tile background maps in VRAM.
     Each can be used either to display "normal" background, or "window" background.
     
     Both background and window can be disabled or enabled separately via bits in the LCDC register.
     */
    const U16 TileMap1Address = 0x9800; // up to 0x9BFF
    const U16 TileMap2Address = 0x9C00; // up to 0x9FFF

    /*
     Tile Data defines the Bitmaps for 192 Tiles.

     Each tile is sized 8x8 pixels and has a color depth of 4 colors/gray shades. Tiles can be 
     displayed as part of the Background/Window map, and/or as OAM tiles (foreground sprites). 
     Note that foreground sprites may have only 3 colors, because color 0 is transparent.

     There are two Tile Pattern Tables at $8000-8FFF and at $8800-97FF. The first one can be used
     for sprites and the background. Its tiles are numbered from 0 to 255. The second table can be
     used for the background and the window display and its tiles are numbered from -128 to 127.

     Each Tile occupies 16 bytes, where each 2 bytes represent a line:

        Byte 0-1  First Line (Upper 8 pixels)
        Byte 2-3  Next Line
        etc.

     For each line, the first byte defines the least significant bits of the color numbers for each
     pixel, and the second byte defines the upper bits of the color numbers. In either case, Bit 7
     is the leftmost pixel, and Bit 0 the rightmost.

     Each pixel has a color number in range from 0-3. The color numbers are translated into real 
     colors (or gray shades) depending on the current palettes. The palettes are defined through 
     registers FF47-FF49.
     */
    const U16 UnsignedTileDataAddress = 0x8000; // up to 0x8FFF
    const U16 SignedTileDataAddress = 0x8800; // up to 0x97FF

    /*
     Only 160x144 pixels can be displayed on the screen. Registers SCROLLX and SCROLLY hold the
     coordinates of background to be displayed in the left upper corner of the screen. Background
     wraps around the screen (i.e. when part of it goes off the screen, it appears on the opposite
     side.)
     */
    const U8 ViewportWidth = 160;
    const U8 ViewportHeight = 144;

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
     
     ^ Stopping LCD operation (bit 7 from 1 to 0) must be performed during V-blank to work properly.
       V- blank can be confirmed when the value of LY is greater than or equal to 144.

     (value 0x91 at reset)
     */
    const U16 LCDC = 0xFF40;

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
    const U16 STAT = 0xFF41;

    /* 
     SCY - Scroll Y (R/W)
     
     8 Bit value $00-$FF to scroll BG Y screen position.
     */
    const U16 SCY = 0xFF42;

    /*
     SCX - Scroll X (R/W)

     8 Bit value $00-$FF to scroll BG X screen position.
     */
    const U16 SCX = 0xFF43;

    /*
     LY - LCDC Y-Coordinate (R)

     The LY indicates the vertical line to which the present data is transferred to the LCD Driver.
     The LY can take on any value between 0 through 153. The values between 144 and 153 indicate
     the V-Blank period. Writing will reset the counter.
     */
    const LY = 0xFF44;

    /*
     LYC - LY Compare (R/W)

     The LYC compares itself with the LY. If the values are the same it causes the STAT to set the
     coincident flag.
     */
    const LYC = 0xFF45;

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
    const U16 DMA = 0xFF46;

    /*
     BGP - BG & Window Palette Data (R/W)

     Bit 7-6 = Data for Dot Data 11 (Normally darkest color)
     Bit 5-4 = Data for Dot Data 10
     Bit 3-2 = Data for Dot Data 01
     Bit 1-0 = Data for Dot Data 00 (Normally lightest color)

     This selects the shade of grays to use for the background (BG) & window pixels. 
     Since each pixel uses 2 bits, the corresponding shade will be selected from here.
     */
    const U16 BGP = 0xFF47;

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
    const U16 WY = 0xFF4A;
    const U16 WX = 0xFF4B;

    /*
     If this bit is set to 0, sprite is displayed on top of background & window. 
     If this bit is set to 1, then sprite will be hidden behind colors 1, 2, and 3 
     of the background & window. (Sprite only prevails over color 0 of BG & win.)
     */
    const U8 SpriteFlag_Priority = (1 << 7);

    // Sprite pattern is flipped vertically if this bit is set to 1. */
    const U8 SpriteFlag_YFlip = (1 << 6);

    // Sprite pattern is flipped horizontally if this bit is set to 1. */
    const U8 SpriteFlag_XFlip = (1 << 5);

    // Sprite colors are taken from OBJ1PAL if this bit is set to 1 and from OBJ0PAL otherwise.
    const U8 SpriteFlag_Palette = (1 << 4);

    struct Sprite {
        U8 y; //Y position on the screen
        U8 x; //X position on the screen
        U8 pattern; //Pattern number 0-255 (Unlike some tile numbers, sprite pattern numbers
                    // are unsigned. LSB is ignored (treated as 0) in 8x16 mode.)
        U8 flags; // see SpriteFlag_* above
    };
} //namespace Video
