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


//const U16 BREAKPOINT = 0x006A; // in boot rom, just after finished wating for vblank
const U16 BREAKPOINT = 0x0000;

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

BGRA greyscale_to_bgra(int greyscale) {
    //GB only has 4 colors
    assert(0 <= greyscale && greyscale <= 3);

    BGRA pixel;
    pixel.a = 0xFF;
    pixel.r = greyscale * 85;
    pixel.g = greyscale * 85;
    pixel.b = greyscale * 85;
    return pixel;
}

void update_texture(SDL_Texture* texture, Bitmap* bitmap) {
    const int bytes_per_pixel = 4;

    U8* pixels = NULL;
    int pitch = 0;
    int lock_result = SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);
    assert(lock_result == 0);

    for(int y = 0; y < bitmap->height; ++y){
        for(int x = 0; x < bitmap->width; ++x){
            BGRA* dest = (BGRA*)(pixels + y*pitch + x*bytes_per_pixel);
            BGRA src = greyscale_to_bgra(bitmap->getPixel(x, y));
            *dest = src;
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
                break;
            default:
                //pass
                break;
        }
    }
    
    printf("\n");
}

void move_window(Emulator* emu, int dx, int dy){
    emu->hardware_registers.wx += dx;
    emu->hardware_registers.wy += dy;
    emu->vram_mutated = True;
    printf("%d/%d\n", (int)emu->hardware_registers.wx, (int)emu->hardware_registers.wy);
}

int main(int argc, const char * argv[]) {
    assert(argc >= 2);

    int init_result = SDL_Init(SDL_INIT_VIDEO);
    assert(init_result == 0);
    atexit(SDL_Quit);

    SDL_Window* window = SDL_CreateWindow("Gemuboi",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          850, 520,
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
                                                 Video::ScreenBufferSize, Video::ScreenBufferSize);
    SDL_Texture* vram_background = SDL_CreateTexture(renderer,
                                                     SDL_PIXELFORMAT_ARGB8888,
                                                     SDL_TEXTUREACCESS_STREAMING,
                                                     Video::ScreenBufferSize, Video::ScreenBufferSize);
    SDL_Texture* vram_tileset = SDL_CreateTexture(renderer,
                                                  SDL_PIXELFORMAT_ARGB8888,
                                                  SDL_TEXTUREACCESS_STREAMING,
                                                  Video::Tileset_PixelsPerRow, Video::Tileset_PixelsPerColumn);
    SDL_Texture* vram_viewport = SDL_CreateTexture(renderer,
                                                   SDL_PIXELFORMAT_ARGB8888,
                                                   SDL_TEXTUREACCESS_STREAMING,
                                                   Video::ViewportWidth, Video::ViewportHeight);


    bool running = true;
    bool continuing = true;
    U32 last_frame = UINT32_MAX;
    while(running){
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_QUIT: running = false; break;
                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym){
                        case SDLK_s:
                            emulator_step(emu);
                            print_next_instruction(emu);
                            break;
                        case SDLK_n: for(int i=0;i<100;++i) emulator_step(emu); break;
                        case SDLK_r: print_register_info(emu); break;
                        case SDLK_c: continuing = true; break;
                        case SDLK_b: continuing = false; break;
                        case SDLK_LEFT: move_window(emu, -1, 0); break;
                        case SDLK_RIGHT: move_window(emu, 1, 0); break;
                        case SDLK_UP: move_window(emu, 0, -1); break;
                        case SDLK_DOWN: move_window(emu, 0, 1); break;
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

        if(continuing && emu->registers.pc == BREAKPOINT){
            printf("Breaking at %0.4X\n", BREAKPOINT);
            continuing = false;
        }

        if(last_frame != emu->gpu.frame_number){
            last_frame = emu->gpu.frame_number;

            SDL_Rect tileset_rect = { 0, 0, Video::Tileset_PixelsPerRow * 2, Video::Tileset_PixelsPerColumn * 2 };
            SDL_Rect window_rect = {tileset_rect.w + 5, 0, Video::ScreenBufferSize, Video::ScreenBufferSize};
            SDL_Rect background_rect = {window_rect.x, window_rect.h + 5, Video::ScreenBufferSize, Video::ScreenBufferSize};
            SDL_Rect viewport_rect = { window_rect.x + window_rect.w + 5, 0, Video::ViewportWidth*2, Video::ViewportHeight*2 };

            update_texture(vram_tileset, &emu->gpu.tileset);
            update_texture(vram_window, &emu->gpu.window);
            update_texture(vram_background, &emu->gpu.background);
            update_texture(vram_viewport, &emu->gpu.viewport);

            SDL_SetRenderDrawColor(renderer, 0, 0x33, 0, 0xFF);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, vram_tileset, NULL, &tileset_rect);
            SDL_RenderCopy(renderer, vram_window, NULL, &window_rect);
            SDL_RenderCopy(renderer, vram_background, NULL, &background_rect);
            SDL_RenderCopy(renderer, vram_viewport, NULL, &viewport_rect);
            SDL_RenderPresent(renderer);
        }
    }

    SDL_DestroyRenderer(renderer);

    return EXIT_SUCCESS;
}

