#include <iostream>
#include "../../msutil.hpp"
#include "ui.hpp"
#include "../geometry/rect.hpp"

class _ColorPicker_Stock {
private:
    static _UIStock stock;
public:
    static Shader *getShader();
};

class ColorPicker : UIElement {
private:
    static _UIStock c_stock;
public:
    void render(graphics *g) override;
    void graphicsPreCompute(graphics *g) override;
};