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
protected:
    Shader s;
public:
    Shader *getShader() {
        return &this->s;
    }
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
    //std::vector<UIElement> elems;
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
private:
    bool computedGraphics = false;
protected:
    UIDimension size; 
    UIScalarDimension bounds;
    f32 zIndex = 0;
    graphicsState gs;
    mat4 u_model;

    virtual void dimUpdate(UIScalarDimension parentBounds);
    virtual void graphicsPreCompute(graphics *g);
public:
    virtual void render(graphics *g);
    virtual void re_render();

    struct MouseState {

        //button that was pressed
        enum class MButton {
            Unknown,
            Left,
            Middle,
            Right
        } mb = MButton::Unknown;

        //emp -> element mouse pos
        //smp -> screen mouse pos
        Point emp;
        Point smp;

        //
        enum class MEventType {
            Unknown,
            Click,
            Down,
            Up,
            Scroll
        } e_ty = MEventType::Unknown;
    };

    //mouse related events
    virtual void mouseDown(MouseState s);
    virtual void mouseUp(MouseState s);
    virtual void mouseHover(MouseState s);
    virtual void mouseClick(MouseState s);
    virtual void mouseScroll(MouseState s);

    //keyboard related events
    virtual void keyDown();
    virtual void keyUp();

    //other events
    virtual void onFocus();
    virtual void offFocus();
};