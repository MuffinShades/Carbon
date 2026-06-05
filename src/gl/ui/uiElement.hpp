#include "ui.hpp"

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
    RenderState gs;
    mat4 u_model;

    virtual void dimUpdate(UIScalarDimension parentBounds) {};
    virtual void graphicsPreCompute(graphics *g);
public:
    virtual void render(graphics *g);
    virtual void re_render() {};

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
    virtual void mouseDown(MouseState s) {};
    virtual void mouseUp(MouseState s) {};
    virtual void mouseHover(MouseState s) {};
    virtual void mouseClick(MouseState s) {};
    virtual void mouseScroll(MouseState s) {};

    //keyboard related events
    virtual void keyDown() {};
    virtual void keyUp() {};

    //other events
    virtual void onFocus() {};
    virtual void offFocus() {};
};