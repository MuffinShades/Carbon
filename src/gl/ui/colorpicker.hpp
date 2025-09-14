#include <iostream>
#include "../../msutil.hpp"
#include "ui.hpp"

class _ColorPicker_Stock : _UIStock {

};

class ColorPicker : UIElement {
private:
    static _UIStock c_stock;
public:
    void render(graphics *g) override;
};