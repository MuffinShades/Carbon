#pragma once
#include <iostream>
#include "../graphics.hpp"
#include "../window.hpp"

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

#define UI_SIMPLE_SHADER_VERT_PATH "../../src/gl/ui/uiShaders/uiSimple_vert.glsl"
#define UI_SIMPLE_SHADER_FRAG_PATH "../../src/gl/ui/uiShaders/uiSimple_frag.glsl"

struct UIDimension {
    USpec x, y, w, h;
};

struct UIScalarDimension {
    f32 x, y, w, h;
};

struct UIColor {
    f32 r,g,b,a;
    enum {
        RGB255,
        RGBA255,
        RGB1,
        RGBA1
    } color_mode = RGBA255;
};

struct UIEvent {
    enum {
        MouseDown,
        MouseUp,
        MouseMove,
        KeyUp,
        KeyDown
    };

    struct {
        i32 btn;
        i32 x,y;
    } mouseData;

    struct {
        i32 key;
    } keyboardData;
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

//max number of unique z orders that a float from 0.0f-1.0f can represent
constexpr i32 MAX_UNIQUE_ZS = 1e6;

class UIObject {
private:
    static i32 next_ui_age;
    static i32 getNextAge();
protected:
    struct {
        UIColor dbgBgColor;
    } _DBG;
    i32 age = -1;
    f32 zorder = 0.0f; //
    vec4 bounds = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    bool focused = false;

    static RenderState *def_object_rs;
    static Shader ui_simple_shader;
public:
    void ini();
    static void load();
    static void close();

    virtual void render(graphics *g, mat4 mmat, vec2 outputDim);
    virtual void update();
    virtual void onFocus();
    virtual void offFocus();
    virtual void preCompute();
    virtual void onEvent(UIEvent ev);
    virtual void resize(UIScalarDimension new_bounds);

    void focus();
    void unfocus();
    mat4 getMatrix();

    friend class UIInst *createUIInst(Window *win);
};

struct SimpleUIVert {
    f32 posf[3];
    f32 color[3];
};