#pragma once

#include "types.hpp"
#include <cassert>

struct Bitmap {
    U16 width;
    U16 height;
    U8* pixels;

    Bitmap(U16 width, U16 height);
    ~Bitmap();
    void clear(U8 color);
    void fillRect(U8 color, U16 x, U16 y, U16 width, U16 height);
    void copyFromBitmap(Bitmap* other, U16 srcX, U16 srcY, U16 dstX, U16 dstY, U16 width, U16 height);
    void setPixel(U16 x, U16 y, U8 color);
    U8 getPixel(U16 x, U16 y);
    
    U8* pixelPtr(U16 x, U16 y){
        assert(x < width);
        assert(y < height);
        return pixels + (y * width) + x;
    }
};