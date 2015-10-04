# pragma once

#include "types.hpp"

namespace Video {
    /* 
     The main GameBoy screen buffer (background) consists of 256x256 pixels
     or 32x32 tiles (8x8 pixels each).
     */
    const U16 ScreenBufferWidth = 256;
    const U16 ScreenBufferHeight = 256;

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

    struct Sprite {
        U8 y; //Y position on the screen
        U8 x; //X position on the screen
        U8 pattern; //Pattern number 0-255 (Unlike some tile numbers, sprite pattern numbers
                    // are unsigned. LSB is ignored (treated as 0) in 8x16 mode.)
        U8 flags; // see SpriteFlag_* above

        /*
         If this bit is set to 0, sprite is displayed on top of background & window.
         If this bit is set to 1, then sprite will be hidden behind colors 1, 2, and 3
         of the background & window. (Sprite only prevails over color 0 of BG & win.)
         */
        static const U8 SpriteFlag_Priority = (1 << 7);

        // Sprite pattern is flipped vertically if this bit is set to 1. */
        static const U8 SpriteFlag_YFlip = (1 << 6);

        // Sprite pattern is flipped horizontally if this bit is set to 1. */
        static const U8 SpriteFlag_XFlip = (1 << 5);

        // Sprite colors are taken from OBJ1PAL if this bit is set to 1 and from OBJ0PAL otherwise.
        static const U8 SpriteFlag_Palette = (1 << 4);
    };

    struct TileMap {
        static const U8 TileSize = 32;
        U8 tiles[TileSize][TileSize];
    };

    struct Tile {
        static const U8 PixelSize = 8;
        struct Row {
            U8 b1;
            U8 b2;
        };
        Row rows[PixelSize];
    };

    union VRAM {
        static const unsigned TileCount = 384;

        U8 memory[0x2000];
        struct {
            // Tile Data Table 1: Tiles 0 - 256
            // Tile Data Table 2: Tiles 128 - 384;
            Tile tiles[TileCount];
            TileMap tilemaps[2];
        };
    };

    union OAM {
        U8 memory[160];
        Video::Sprite sprites[40];
    };

} //namespace Video
