#include <iostream>
#include "../../msutil.hpp"
#include "ui.hpp"
#include "../geometry/rect.hpp"

struct ColorPick_Vertex {
    f32 pos[2];
    f32 color[3];
};

class _ColorPicker_Stock {
private:
    static _UIStock stock;
public:
    static Shader *getShader();
    
};

class ColorPicker : public UIObject {
private:
    static _UIStock c_stock;
    ColorPick_Vertex sv_rect[4];
public:
    void render(graphics *g, mat4 mmat, vec2 outputDim) override;
    void preCompute() override;
};