//
//  timer.cpp
//  gemuboi
//
//  Created by Tom on 26/09/2015.
//
//

#include "bitmap.hpp"
#include <cstring>

Bitmap::Bitmap(U16 w, U16 h) :
    width(w),
    height(h)
{
    assert(width > 0);
    assert(height > 0);
    pixels = new U8[width * height];
}

Bitmap::~Bitmap() {
    if(pixels)
        delete[] pixels;
}

void Bitmap::clear(U8 color){
    fillRect(color, 0, 0, width, height);
}

void Bitmap::fillRect(U8 color, U16 rx, U16 ry, U16 rwidth, U16 rheight) {
    if(rwidth == 0 || rheight == 0)
        return;

    U16 rmaxx = rx + rwidth;
    U16 rmaxy = ry + rheight;

    for(unsigned x = rx; x < rmaxx; ++x){
        for(unsigned y = ry; y < rmaxy; ++y){
            U8* pixel = pixelPtr(x, y);
            *pixel = color;
        }
    }
}

void Bitmap::setPixel(U16 x, U16 y, U8 color) {
    U8* pixel = pixelPtr(x, y);
    *pixel = color;
}

U8 Bitmap::getPixel(U16 x, U16 y) {
    U8* pixel = pixelPtr(x, y);
    return *pixel;
}

void Bitmap::copyFromBitmap(Bitmap *other, U16 srcX, U16 srcY, U16 dstX, U16 dstY, U16 width, U16 height){
    for(U16 x = 0; x < width; ++x){
        for(U16 y = 0; y < height; ++y){
            U8* srcPixel = other->pixelPtr(srcX + x, srcY + y);
            U8* dstPixel = pixelPtr(dstX + x, dstY + y);
            *dstPixel = *srcPixel;
        }
    }
}