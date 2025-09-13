#include <iostream>
#include "../../msutil.hpp"
#include "ui.hpp"

class _ColorPicker_Stock : _UIStock {

};

class ColorPicker {
private:
    static _UIStock c_stock;
public:
    void render(f32 x, f32 y);
};