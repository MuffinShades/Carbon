#include <iostream>
#include "../graphics.hpp"

class USpec {
public:
    f32 v = 0;
    enum class Mode {
        scalar,
        percent
    } form = Mode::scalar;
    USpec(f32 v, Mode f) {
        this->v = v;
        this->form = f;
    }
};

#define UI_VAL_PERCENT(v) USpec(v, USpec::Mode::percent)
#define UI_VAL_SCALAR(v) USpec(v, USpec::Mode::scalar)

struct UIDimension {
    USpec x, y, w, h;
};

struct UIScalarDimension {
    f32 x, y, w, h;
};

/**
 * 
 * UIStock
 * 
 * Should have a static of this in each ui element's class
 * to specify shaders and other stuff that all instances of
 * a given element can share
 * 
 */
class _UIStock {
public:
    Shader s;
};

/**
 * 
 * Top Leve ui element that is directly connected to the window
 * 
 * This class contqins many important things such as the general
 * projection matrix for all the ui elements of a window and much
 * more
 * 
 */
class UIWin {
private:
    static mat4 ui_proj;
};

/**
 * 
 * UIContainer
 * 
 * base class of ui elements that are types of containers
 * such as group boxes, general containers, windows, and
 * other types of containers
 * 
 */
class UIContainer {
public:
    enum class OverflowMode {
        None,
        Hide,
        Scroll,
        Adjust
    };
private:
    UIDimension bounds;
    OverflowMode o_mode = OverflowMode::None;
    std::vector<UIElement> elems;
public:
    void render(graphics *g);
};

/**
 * 
 * UIElement
 * 
 * Base class for all non-container ui elements such as
 * colorpicker, text/number input, text display, checkboxes
 * etc.
 * 
 */
class UIElement {
protected:
    UIDimension size; 
    UIScalarDimension bounds;
    f32 zIndex = 0;
    graphicsState gs;
    mat4 u_model;

    bool computedGraphics = false;

    virtual void dimUpdate(UIScalarDimension parentBounds);
public:
    virtual void render(graphics *g);
};