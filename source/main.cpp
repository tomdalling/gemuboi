//
//  main.cpp
//  gemuboi
//
//  Created by Tom on 17/09/2015.
//
//
#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

#include <SDL2/SDL.h>

#include "types.hpp"
#include "cart.hpp"
#include "cpu.hpp"
#include "emulator.hpp"

const unsigned Tileset_TilesPerRow = 16;
const unsigned Tileset_TilesPerColumn = (Video::VRAM::TileCount / Tileset_TilesPerRow);
const unsigned Tileset_PixelsPerRow = Tileset_TilesPerRow * Video::Tile::PixelSize;
const unsigned Tileset_PixelsPerColumn = Tileset_TilesPerColumn * Video::Tile::PixelSize;

const unsigned Tilemap_PixelSize = Video::TileMap::TileSize * Video::Tile::PixelSize;

void cart_fread(Cart::Cart* cart, const char* filename) {
    FILE* f = fopen(filename, "rb");
    assert(f);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    assert(size <= sizeof(Cart::Cart));
    rewind(f);
    fread(cart, size, 1, f);
    fclose(f);
}

void cart_get_title(Cart::Cart* cart, char* out_title) {
    strncpy(out_title, (const char*)&(cart->header.game_title), Cart::TitleSize - 1);
    out_title[Cart::TitleSize - 1] = 0;
}

void test(Emulator* emu) {
    // check typedef'd sizes
    assert(sizeof(U8) == 1);
    assert(sizeof(U16) == 2);
    assert(sizeof(U32) == 4);

    // check 16bit register byte order
    emu->registers.hl = 0xABCD;
    assert(emu->registers.h == 0xAB);
    assert(emu->registers.l == 0xCD);
    emu->registers.hl = 0;

    // check byte order of U16 (least significant first)
    U16 u16 = 0xABCD;
    U8* u16p = (U8*)(void*)&u16;
    assert(u16p[0] == 0xCD);
    assert(u16p[1] == 0xAB);

    //check sizes of structs
    assert(sizeof(Video::Sprite) == 4);
    assert(sizeof(Video::Tile) == 16);
    assert(sizeof(Video::TileMap) == 1024);
    assert(sizeof(Video::VRAM) == 0x2000);
    assert(sizeof(Video::OAM) == 160);
}

struct BGRA {
    U8 b;
    U8 g;
    U8 r;
    U8 a;
};

// returns 0, 1, 2, or 3
U8 unpack_pixel(U8 first_byte, U8 second_byte, U8 pixel_idx){
    U8 bit0 = ((first_byte >> pixel_idx) & 0x01);
    U8 bit1 = ((second_byte >> pixel_idx) & 0x01) << 1;
    return (bit0 | bit1);
}

void blit_tile(Video::Tile* tile, void* dest, int dest_pitch, unsigned dest_x, unsigned dest_y) {
    assert(dest_pitch > 0);

    const unsigned bytes_per_texture_pixel = 4;

    for(unsigned tile_row = 0; tile_row < Video::Tile::PixelSize; ++tile_row){
        unsigned texture_y = dest_y + tile_row;
        BGRA* row = (BGRA*)((U8*)dest + dest_pitch*texture_y + dest_x*bytes_per_texture_pixel);
        for(unsigned pixel_idx = 0; pixel_idx < Video::Tile::PixelSize; ++pixel_idx){
            Video::Tile::Row& packed_row = tile->rows[tile_row];
            U8 greyscale = unpack_pixel(packed_row.b1, packed_row.b2, 7 - pixel_idx);
            BGRA &pixel = row[pixel_idx];
            pixel.a = 0xFF;
            pixel.r = greyscale * 85;
            pixel.g = greyscale * 85;
            pixel.b = greyscale * 85;
        }
    }

}

void update_tileset(SDL_Texture* texture, Video::VRAM* vram) {
    void* buffer = NULL;
    int pitch = 0;
    int did_lock = SDL_LockTexture(texture, NULL, &buffer, &pitch);
    assert(did_lock == 0);

    for(unsigned tile_idx = 0; tile_idx < Video::VRAM::TileCount; ++tile_idx){
        unsigned texture_x = (tile_idx % Tileset_TilesPerRow) * Video::Tile::PixelSize;
        unsigned texture_y = Video::Tile::PixelSize * (tile_idx / Tileset_TilesPerRow);
        blit_tile(&vram->tiles[tile_idx], buffer, pitch, texture_x, texture_y);
    }

    SDL_UnlockTexture(texture);
}

void update_tilemap(SDL_Texture* texture, Video::VRAM* vram, unsigned tilemap_idx) {
    void* buffer = NULL;
    int pitch = 0;
    int did_lock = SDL_LockTexture(texture, NULL, &buffer, &pitch);
    assert(did_lock == 0);

    for(unsigned y = 0; y < Video::TileMap::TileSize; ++y){
        for(unsigned x = 0; x < Video::TileMap::TileSize; ++x){
            U8 tile_idx = vram->tilemaps[tilemap_idx].tiles[y][x];
            unsigned px = x * Video::Tile::PixelSize;
            unsigned py = y * Video::Tile::PixelSize;
            blit_tile(&vram->tiles[tile_idx], buffer, pitch, px, py);
        }
    }

    SDL_UnlockTexture(texture);
}

void print_register_info(Emulator* emu) {
    printf("======================\n"
           "AF %0.4X       A %0.2X\n"
           "BC %0.4X\n"
           "DE %0.4X\n"
           "HL %0.4X    (HL) %0.2X\n",
           (unsigned)emu->registers.af, (unsigned)emu->registers.a,
           (unsigned)emu->registers.bc,
           (unsigned)emu->registers.de,
           (unsigned)emu->registers.hl, (unsigned)emu->mem_read(emu->registers.hl)
           );
}

void print_next_instruction(Emulator* emu) {
    const U8 instr[3] = {
        emu->mem_read(emu->registers.pc),
        emu->mem_read(emu->registers.pc + 1),
        emu->mem_read(emu->registers.pc + 2)
    };

    // instruction address
    printf("%0.4X: ", emu->registers.pc);

    if(instr[0] == 0xCB){
        // pseudo ASM
        printf("0xCB 0x%0.2X %s", (unsigned)instr[1], CPU::CBPrefixedOpcodes[instr[1]].description);
    } else {
        const CPU::OpcodeDesc& opcode = CPU::Opcodes[instr[0]];
        // pseudo ASM
        printf("0x%0.2X %s ", (unsigned)*instr, opcode.description);

        // direct operands
        switch(opcode.byte_length){
            case 2:
                printf("[0x%0.2X]", instr[1]);
                break;
            case 3:
                printf("[0x%0.2X%0.2X]", instr[2], instr[1]);
            default:
                //pass
                break;
        }

    }
    
    printf("\n");
}

int main(int argc, const char * argv[]) {
    assert(argc >= 2);

    int init_result = SDL_Init(SDL_INIT_VIDEO);
    assert(init_result == 0);
    atexit(SDL_Quit);

    SDL_Window* window = SDL_CreateWindow("Gemuboi",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          800, 600,
                                          SDL_WINDOW_SHOWN);
    assert(window);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    assert(renderer);

    Emulator* emu = new Emulator;
    emulator_init(emu);
    cart_fread(&emu->cart, argv[1]);

    test(emu);

    SDL_Texture* vram_window = SDL_CreateTexture(renderer,
                                                 SDL_PIXELFORMAT_ARGB8888,
                                                 SDL_TEXTUREACCESS_STREAMING,
                                                 Tilemap_PixelSize, Tilemap_PixelSize);
    SDL_Texture* vram_background = SDL_CreateTexture(renderer,
                                                     SDL_PIXELFORMAT_ARGB8888,
                                                     SDL_TEXTUREACCESS_STREAMING,
                                                     Tilemap_PixelSize, Tilemap_PixelSize);
    SDL_Texture* vram_tileset = SDL_CreateTexture(renderer,
                                                  SDL_PIXELFORMAT_ARGB8888,
                                                  SDL_TEXTUREACCESS_STREAMING,
                                                  Tileset_PixelsPerRow, Tileset_PixelsPerColumn);

    bool running = true;
    bool continuing = false;
    while(running){
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_QUIT: running = false; break;
                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym){
                        case SDLK_s: emulator_step(emu); break;
                        case SDLK_n: for(int i=0;i<100;++i) emulator_step(emu); break;
                        case SDLK_r: print_register_info(emu); break;
                        case SDLK_c: continuing = true; break;
                        case SDLK_b: continuing = false; break;
                        default: break; //do nothing
                    }
                    break;
            }
        }

        if(continuing){
            emulator_step(emu);
        } else {
            SDL_Delay(1); //don't check up the CPU too badly
        }

        if(emu->vram_mutated){
            emu->vram_mutated = False;

            update_tileset(vram_tileset, &emu->vram);
            update_tilemap(vram_window, &emu->vram, 0);
            update_tilemap(vram_background, &emu->vram, 1);

            SDL_Rect tileset_rect = { 0, 0, Tileset_PixelsPerRow * 2, Tileset_PixelsPerColumn * 2 };
            SDL_Rect window_rect = {tileset_rect.w + 5, 0, Tilemap_PixelSize, Tilemap_PixelSize};
            SDL_Rect background_rect = {window_rect.x, window_rect.h + 5, Tilemap_PixelSize, Tilemap_PixelSize};

            SDL_SetRenderDrawColor(renderer, 0, 0x33, 0, 0xFF);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, vram_tileset, NULL, &tileset_rect);
            SDL_RenderCopy(renderer, vram_window, NULL, &window_rect);
            SDL_RenderCopy(renderer, vram_background, NULL, &background_rect);
            SDL_RenderPresent(renderer);
        }
    }

    SDL_DestroyRenderer(renderer);

    return EXIT_SUCCESS;
}

