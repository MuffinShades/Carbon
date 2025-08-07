#include "Texture.hpp"
#include "../../gl_lib/glad/include/glad/glad.h"
#include "../game/assetManager.hpp"

BindableTexture::BindableTexture(std::string isrc) {

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

    if (!img.data || img.sz == 0) {
        std::cout << "Failed to read png: " << tex_id << std::endl;

        if (img.data)
            _safe_free_a(img.data);

        return;
    }

    //now generate the texture
    glGenTextures(1, &this->t_handle);

    if (!this->t_handle) {
        std::cout << "error: failed to create texture!" << std::endl;
        _safe_free_a(img.data);
        return;
    }

    glBindTexture(GL_TEXTURE_2D, this->t_handle);

    //image params
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    if (img.bitDepth != 24 || img.bitDepth != 32) {
        std::cout << "error: usupported gl bit depth of " << img.bitDepth << std::endl;
         _safe_free_a(img.data);
        return;
    } 

    auto gl_fmt = img.bitDepth == 24 ? GL_RGB : GL_RGBA;

    //load texture data and mipmap
    glTexImage2D(GL_TEXTURE_2D, 0, gl_fmt, img.width, img.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data);
    glGenerateMipmap(GL_TEXTURE_2D);

    //free image memory now
    _safe_free_a(img.data);
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