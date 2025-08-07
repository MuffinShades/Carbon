#include <iostream>
#include "../png.hpp"

class BindableTexture {
private:
    u32 t_handle = 0;
public:
    BindableTexture(std::string isrc);
    BindableTexture(std::string asset_path, std::string map_loc, std::string tex_loc);

    void bind(u32 slot = 0);
    u32 getHandle() const;
};