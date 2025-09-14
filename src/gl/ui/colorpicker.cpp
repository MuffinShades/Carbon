#include "colorpicker.hpp"

void ColorPicker::render(graphics *g) {
    if (!g)
        return;

    if (!this->computedGraphics) {
        g->useGraphicsState(&this->gs);
        g->iniStaticGraphicsState();
        
    }
}