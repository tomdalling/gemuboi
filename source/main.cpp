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

const unsigned VramWidth = 128;  // 16 tiles
const unsigned VramHeight = 196; // 24 tiles

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

void update_vram(SDL_Texture* texture, Emulator* emu) {
    const unsigned tile_size = 8;
    const unsigned tiles_per_row = (VramWidth / tile_size);
    const unsigned tile_count = 128 * 3;
    const unsigned bytes_per_texture_pixel = 4;

    void** texture_data = NULL;
    int pitch = 0;
    int did_lock = SDL_LockTexture(texture, NULL, (void**)&texture_data, &pitch);
    assert(did_lock == 0);

    for(unsigned tile_idx = 0; tile_idx < tile_count; ++tile_idx){
        unsigned texture_x = (tile_idx % tiles_per_row) * tile_size * bytes_per_texture_pixel;
        for(unsigned tile_row = 0; tile_row < tile_size; ++tile_row){
            unsigned texture_y = tile_size*(tile_idx / tiles_per_row) + tile_row;
            BGRA* row = (BGRA*)((U8*)texture_data + pitch*texture_y + texture_x);
            for(unsigned pixel_idx = 0; pixel_idx < tile_size; ++pixel_idx){
                Video::Tile::Row& packed_row = emu->vram.tiles[tile_idx].rows[tile_row];
                U8 greyscale = unpack_pixel(packed_row.b1, packed_row.b2, 7 - pixel_idx);
                BGRA &pixel = row[pixel_idx];
                pixel.a = 0xFF;
                pixel.r = greyscale * 85;
                pixel.g = greyscale * 85;
                pixel.b = greyscale * 85;
            }
        }
    }

    SDL_UnlockTexture(texture);
}

void print_debug_info(Emulator* emu) {
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

int main(int argc, const char * argv[]) {
    assert(argc >= 2);

    int init_result = SDL_Init(SDL_INIT_VIDEO);
    assert(init_result == 0);
    atexit(SDL_Quit);

    SDL_Window* window = SDL_CreateWindow("Gemuboi",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          VramWidth*3, VramHeight*3,
                                          SDL_WINDOW_SHOWN);
    assert(window);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    assert(renderer);

    Emulator* emu = new Emulator;
    emulator_init(emu);
    cart_fread(&emu->cart, argv[1]);

    test(emu);

    SDL_Texture* vram = SDL_CreateTexture(renderer,
                                          SDL_PIXELFORMAT_ARGB8888,
                                          SDL_TEXTUREACCESS_STREAMING,
                                          VramWidth, VramHeight);
    SDL_SetTextureBlendMode(vram, SDL_BLENDMODE_NONE);
    SDL_SetTextureColorMod(vram, 0, 0, 0);
    SDL_SetTextureAlphaMod(vram, 0);

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
                        case SDLK_r: print_debug_info(emu); break;
                        case SDLK_c: continuing = true; break;
                        case SDLK_b: continuing = false; break;
                        default: break; //do nothing
                    }
                    break;
            }
        }

        if(continuing){
            emulator_step(emu);
        }

        update_vram(vram, emu);

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 0);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, vram, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);

    return EXIT_SUCCESS;
}

