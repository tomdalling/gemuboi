#pragma once

#include <cstdlib>
#include "types.hpp"

namespace Cart {
    const U8 NintendoLogo[48] = {
        0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
        0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
        0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
        0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
        0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
        0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
    };

    const size_t MaxSize = 8 * 1024 * 1024; //8MiB
    const size_t TitleSize = 17; //16 bytes + null terminator

    struct Header {
        // 0000-0099
        // arbitrary instructions (interrupt handlers)
        U8 interrupts[256];

        // 0100-0103
        // cpu jumps to here when game starts
        U8 entry_point[4];

        // 0104-0133
        // must be equal to `NintendoLogo`
        U8 nintendo_logo[48];

        // 0134-013E
        // can spill into the next two elements (up to 16 chars)
        U8 game_title[11];

        // 013F-0142
        // 4 byte manufacturer identifier
        U8 manufaturer_code[4];

        // 0144
        // upper bit enables CGB functions. Typical values are:
        //   0x80 = supports CGB functions, but works on old gameboys also.
        //   0xC0 = works on CGB only (physically the same as 0x80).
        U8 cgb_flag;

        // 0144-0145
        // indicates the company or publisher of the game. These two bytes are
        // used in newer games only. Older games are using the header entry at
        // 0x014B instead.
        U8 new_licensee_code[2];

        // 0146
        // Specifies whether the game supports SGB functions, common values
        // are:
        //   0x00 = No SGB functions (Normal Gameboy or CGB only game)
        //   0x03 = Game supports SGB functions
        U8 sgb_flag;

        // 0147
        // Specifies which Memory Bank Controller (if any) is used in the
        // cartridge, and if further external hardware exists in the cartridge.
        //   0x00  ROM ONLY                 0x13  MBC3+RAM+BATTERY
        //   0x01  MBC1                     0x15  MBC4
        //   0x02  MBC1+RAM                 0x16  MBC4+RAM
        //   0x03  MBC1+RAM+BATTERY         0x17  MBC4+RAM+BATTERY
        //   0x05  MBC2                     0x19  MBC5
        //   0x06  MBC2+BATTERY             0x1A  MBC5+RAM
        //   0x08  ROM+RAM                  0x1B  MBC5+RAM+BATTERY
        //   0x09  ROM+RAM+BATTERY          0x1C  MBC5+RUMBLE
        //   0x0B  MMM01                    0x1D  MBC5+RUMBLE+RAM
        //   0x0C  MMM01+RAM                0x1E  MBC5+RUMBLE+RAM+BATTERY
        //   0x0D  MMM01+RAM+BATTERY        0xFC  POCKET CAMERA
        //   0x0F  MBC3+TIMER+BATTERY       0xFD  BANDAI TAMA5
        //   0x10  MBC3+TIMER+RAM+BATTERY   0xFE  HuC3
        //   0x11  MBC3                     0xFF  HuC1+RAM+BATTERY
        //   0x12  MBC3+RAM
        U8 hardware;

        // 0148
        // Specifies the ROM Size of the cartridge. Typically calculated as
        // "32KB shl N".
        //   0x00 =  32KByte (no ROM banking)
        //   0x01 =  64KByte (4 banks)
        //   0x02 = 128KByte (8 banks)
        //   0x03 = 256KByte (16 banks)
        //   0x04 = 512KByte (32 banks)
        //   0x05 =   1MByte (64 banks)  - only 63 banks used by MBC1
        //   0x06 =   2MByte (128 banks) - only 125 banks used by MBC1
        //   0x07 =   4MByte (256 banks)
        //   0x52 = 1.1MByte (72 banks)
        //   0x53 = 1.2MByte (80 banks)
        //   0x54 = 1.5MByte (96 banks)
        U8 rom_size;

        // 0149
        // Specifies the size of the external RAM in the cartridge (if any).
        //   0x00 = None
        //   0x01 = 2 KBytes
        //   0x02 = 8 Kbytes
        //   0x03 = 32 KBytes (4 banks of 8KBytes each)
        //   0x04 = 128 KBytes (16 banks of 8KBytes each)
        //   0x05 = 64 KBytes (8 banks of 8KBytes each)
        U8 ram_size;

        // 014A
        // Specifies if this version of the game is supposed to be sold in
        // japan, or anywhere else. Only two values are defined.
        //   0x00 = Japanese
        //   0x01 = Non-Japanese
        U8 destination_code;

        // 014B
        // Specifies the games company/publisher code in range 0x00-0xFF. A
        // value of 0x33 signalizes that the New License Code in header bytes
        // 0144-0145 is used instead. (Super GameBoy functions won't work if !=
        // 0x33.)
        U8 old_licensee_code;

        // 014C
        // Specifies the version number of the game. That is usually 0x00.
        U8 version_number;

        // 014D
        // Contains an 8 bit checksum across the cartridge header bytes
        // 0134-014C. The checksum is calculated as follows:
        //   x=0:FOR i=0x0134 TO 0x014C:x=x-MEM[i]-1:NEXT
        // The lower 8 bits of the result must be the same than the value in
        // this entry. The GAME WON'T WORK if this checksum is incorrect.
        U8 header_checksum;

        // 014E-014F
        // Contains a 16 bit checksum (upper byte first) across the whole
        // cartridge ROM. Produced by adding all bytes of the cartridge (except
        // for the two checksum bytes). The Gameboy doesn't verify this
        // checksum.
        U8 global_checksum[2];
    };

    union Cart {
        Header header;
        U8 data[MaxSize];
    };
}

