//
//  timer.cpp
//  gemuboi
//
//  Created by Tom on 26/09/2015.
//
//

#include "video.hpp"


// returns 0, 1, 2, or 3
U8 Video::Tile::Row::unpack_pixel(U8 pixel_idx) {
    U8 bit0 = ((b1 >> pixel_idx) & 0x01);
    U8 bit1 = ((b2 >> pixel_idx) & 0x01) << 1;
    return (bit0 | bit1);
}


Video::GPU::GPU() :
    viewport(ViewportWidth, ViewportHeight),
    window(ScreenBufferSize, ScreenBufferSize),
    background(ScreenBufferSize, ScreenBufferSize),
    tileset(Tileset_PixelsPerRow, Tileset_PixelsPerColumn)
{
    viewport.clear(0);
    window.clear(1);
    background.clear(2);
    tileset.clear(3);
}

void Video::GPU::step(U8 cycles) {
    BOOL32 redraw = step_mode(cycles);
    if(redraw){
        update_tileset();
        update_tilemap(&window, 0);
        update_tilemap(&background, 1);
        update_viewport();
    }
}

BOOL32 Video::GPU::step_mode(U8 cycles) {
    BOOL32 redraw = False;
    cycles_elapsed += cycles;

    if(cycles_elapsed >= GPUModeDurations[mode]){
        cycles_elapsed -= GPUModeDurations[mode];

        switch(mode){
            case OAM_READ_MODE:
                mode = VRAM_READ_MODE;
                break;

            case VRAM_READ_MODE:
                mode = HBLANK_MODE;
                break;

            case HBLANK_MODE:
                line += 1;
                if(line == ViewportHeight){
                    // last line rendered, so enter vblank
                    mode = VBLANK_MODE;
                    frame_number++;
                    redraw = True;
                } else {
                    // move to next line
                    mode = OAM_READ_MODE;
                }
                break;

            case VBLANK_MODE:
                line += 1;
                //TODO: check that the vblank mode actually runs for the correct number of cycles
                if(line == ViewportHeight + VBlankLines){
                    // start again
                    line = 0;
                    mode = OAM_READ_MODE;
                } else {
                    // mode stays the same
                }
                break;
        }
    }
    
    return redraw;
}

void Video::GPU::update_tileset() {
    for(unsigned tile_idx = 0; tile_idx < VRAM::TileCount; ++tile_idx){
        U16 texture_x = (tile_idx % Tileset_TilesPerRow) * Tile::PixelSize;
        U16 texture_y = Tile::PixelSize * (tile_idx / Tileset_TilesPerRow);
        blit_tile(&vram.tiles[tile_idx], &tileset, texture_x, texture_y);
    }
}

void Video::GPU::blit_tile(Tile* tile, Bitmap* bitmap, U16 x, U16 y) {
    for(unsigned tile_row_idx = 0; tile_row_idx < Tile::PixelSize; ++tile_row_idx){
        Video::Tile::Row& tile_row = tile->rows[tile_row_idx];
        U8* bitmap_row = bitmap->pixelPtr(x, y+tile_row_idx);
        for(unsigned pixel_idx = 0; pixel_idx < Video::Tile::PixelSize; ++pixel_idx){
            bitmap_row[pixel_idx] = tile_row.unpack_pixel(7 - pixel_idx);
        }
    }
}

void Video::GPU::update_tilemap(Bitmap* bitmap, U8 tilemap_idx) {
    for(unsigned y = 0; y < TileMap::TileSize; ++y){
        for(unsigned x = 0; x < TileMap::TileSize; ++x){
            U8 tile_idx = vram.tilemaps[tilemap_idx].tiles[y][x];
            unsigned destx = x * Tile::PixelSize;
            unsigned desty = y * Tile::PixelSize;
            blit_tile(&vram.tiles[tile_idx], bitmap, destx, desty);
        }
    }
}

void Video::GPU::update_viewport() {
    //TODO: make sure all the LCDC flags are respected

    // clear the background
//    BGRA clear_color = greyscale_to_bgra(0);
//    SDL_SetRenderDrawColor(renderer, clear_color.r, clear_color.g, clear_color.b, clear_color.a);
//    SDL_RenderFillRect(renderer, &dest);
//
//    if(!hw_regs->lcdc_display_enabled()){
//        return;
//    }
//
//    if(hw_regs->lcdc_background_enabled()){
//        SDL_Rect bg_src = { hw_regs->scx, hw_regs->scy, Video::ViewportWidth, Video::ViewportHeight };
//        SDL_RenderCopy(renderer, background, &bg_src, &dest);
//    }
//
//    if(hw_regs->lcdc_window_enabled()){
//        const int WX_OFFSET = 7;
//        int wx = hw_regs->wx;
//        int wy = hw_regs->wy;
//        if(wx <= Video::ViewportWidth + WX_OFFSET - 1 && wy <= Video::ViewportHeight - 1){
//            SDL_Rect window_src = {
//                wx < WX_OFFSET ? WX_OFFSET - wx : 0,
//                0,
//                Video::ViewportWidth - (wx < WX_OFFSET ? 0 : wx - WX_OFFSET),
//                Video::ViewportHeight - wy,
//            };
//            SDL_Rect window_dest = {
//                dest.x + 2*(wx < WX_OFFSET ? 0 : wx - WX_OFFSET),
//                dest.y + 2*wy,
//                2*window_src.w,
//                2*window_src.h,
//            };
//            SDL_RenderCopy(renderer, window, &window_src, &window_dest);
//        }
//    }
}

