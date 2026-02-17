#pragma once
#include <iostream>
#include "../png.hpp"
#include "../bitmap.hpp"

class BindableTexture {
private:
    u32 t_handle = 0;
    static u32 GenTexFromDecodedPng(BindableTexture* tex, png_image i);
    static u32 GenTexFromBitmap(BindableTexture* self, Bitmap *bmp);
    u32 w, h;
public:
    BindableTexture(std::string isrc);
    BindableTexture(std::string asset_path, std::string map_loc, std::string tex_loc);
    BindableTexture(u32 handle, u32 w, u32 h);
    BindableTexture(Bitmap *bmp);
    BindableTexture() {}

    u32 width() const;
    u32 height() const;

    void bind(u32 slot = 0);
    u32 getHandle() const;

    void free();
};