#pragma once
#include "ui.hpp"
#include "uiWin.hpp"
#include "../window.hpp"
#include <vector>

class UIInst {
private:
    std::vector<UIWin*> c_wins;
    mat4 ui_proj;
    vec2 ext_pos = vec2(0.0f, 0.0f);

    graphics *g = nullptr;
    RenderState pDefState;

    void __int_delete();
    i64 __int_addUWin(UIWin *win);
    void __int_notifWinDestroyed(size_t loc);

    FrameBuffer objLocBuffer;

    u32 output_w = 0, output_h = 0;

    UIObject bgObj;
public:
    void attachToWin(Window *win, f32 x, f32 y, f32 w, f32 h);

    void mouseDown(f32 x, f32 y);
    void mouseMove(f32 x, f32 y);
    void mouseUp(f32 x, f32 y);
    void keyUp(i32 code);
    void keyDown(i32 code);

    void update();
    void rerenderAll();

    UIInst() {
    }

    friend void deleteUIInst(UIInst *inst);
    friend UIInst *createUIInst(Window *win);
    friend class UIWin;
};

extern UIInst *createUIInst(Window *win);
extern void deleteUIInst(UIInst *inst);