#include "Texture.hpp"
#include "../../gl_lib/glad/include/glad/glad.h"
#include "../game/assetManager.hpp"

u32 BindableTexture::GenTexFromDecodedPng(BindableTexture* self, png_image img) {
    if (!img.data || img.sz == 0) {
        std::cout << "invalid texture i_data!" << std::endl;
        if (img.data)
            _safe_free_a(img.data);

        return 1;
    }

    if (!self) {
        std::cout << "invalid self ;-;" << std::endl;
        return 4;
    }

    std::cout << "handle address: " << (uintptr_t) (&self->t_handle) << std::endl;

    //now generate the texture
    glGenTextures(1, &self->t_handle);

    if (!self->t_handle) {
        std::cout << "error: failed to create texture!" << std::endl;
        _safe_free_a(img.data);
        return 2;
    }

    glBindTexture(GL_TEXTURE_2D, self->t_handle);

    //image params
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    if (img.channels != 3 && img.channels != 4) {
        std::cout << "error: usupported number of channels: " << img.channels << std::endl;
         _safe_free_a(img.data);
        return 3;
    }

    auto gl_fmt = img.channels == 3 ? GL_RGB : GL_RGBA;

    //load texture data and mipmap
    glTexImage2D(GL_TEXTURE_2D, 0, gl_fmt, img.width, img.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0); //unbind from le texture

    //free image memory now
    _safe_free_a(img.data);

    self->w = img.width;
    self->h = img.height;

    return 0;
}

BindableTexture::BindableTexture(std::string isrc) {
    if (isrc.length() == 0) return;

    png_image img = PngParse::Decode(Path::GetOSPath(isrc));

    u32 e_code;

    if (e_code = GenTexFromDecodedPng(this, img)) {
        std::cout << "Failed to gen texture: " << isrc << " | err code: " << e_code << std::endl;
        return;
    }
}

BindableTexture::BindableTexture(std::string asset_path, std::string map_loc, std::string tex_id) {
    Asset *tex_dat = AssetManager::ReqAsset(tex_id, asset_path, map_loc);

    //AssetManager::ReqAsset(vert_id, asset_path, map_loc)

    if (!tex_dat) {
        std::cout << "Failed to load texture: " << tex_id << std::endl;
        return;
    }

    //extract the raw image data
    //TODO: support  other image formats besides .png
    png_image img = PngParse::DecodeBytes(tex_dat->bytes, tex_dat->sz);

    tex_dat->free();
    _safe_free_b(tex_dat);

    u32 e_code;

    if (e_code = GenTexFromDecodedPng(this, img)) {
        std::cout << "Failed to gen texture: " << tex_id << " | err code: " << e_code << std::endl;
        return;
    }
}

void BindableTexture::bind(u32 slot) {
    if (slot >= 16) {
        std::cout << "Warning: cannot bind to texture slot above 15! Trying to bind to slot: " << slot << std::endl;
        return;
    }

    if (!this->t_handle) {
        std::cout << "Warning: cannot bind to uncreated texture!" << std::endl;
        return;
    }

    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, this->t_handle);
}

u32 BindableTexture::getHandle() const {
    return this->t_handle;
}

u32 BindableTexture::width() const {
    return this->w;
}

u32 BindableTexture::height() const {
    return this->h;
}