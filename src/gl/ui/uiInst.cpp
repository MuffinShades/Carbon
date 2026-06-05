#include "uiInst.hpp"

void UIInst::update() {
    for (UIWin *u : this->c_wins) {
        if (!u) continue; //ensure win wasnt deleted

        u->update();
    }
}

UIInst *createUIInst(Window *win) {
    UIInst *inst = new UIInst();

    inst->attachToWin(win, 0.0f, 0.0f, win->w, win->h);

    inst->pDefState = {

    };

    //TODO: configure the default render state
    inst->g = new graphics(&inst->pDefState);

    inst->output_w = win->w;
    inst->output_h = win->h;

    //load inst's bg
    inst->bgObj.ini();
    inst->bgObj.resize(UIScalarDimension(0.0f, 0.0f, inst->output_w, inst->output_h));
    inst->bgObj._DBG.dbgBgColor.r = 0.5f;
    inst->bgObj._DBG.dbgBgColor.g = 0.0f;
    inst->bgObj._DBG.dbgBgColor.b = 0.5f;
    
    return inst;
}

void deleteUIInst(UIInst *inst) {
    if (!inst) return;

    inst->__int_delete();
    _safe_free_b(inst);
}

void UIInst::__int_delete() {
    //TODO: delete all children first (do more than just close or something idk)
    for (UIWin *u : this->c_wins) {
        if (!u) continue; //ensure win wasnt deleted

        //u->close();
    }

    //TODO: ensure no one has or saves this pointer
    _safe_free_b(this->g);
    this->g = nullptr;
}

void UIInst::attachToWin(Window *win, f32 x, f32 y, f32 w, f32 h) {
    if (!win) return;

    this->ui_proj = mat4::CreateOrthoProjectionMatrix(w, 0.0f, h, 0.0f, -1.0f, 1.0f);
    this->ext_pos = vec2(x,y);
}

void UIInst::rerenderAll() {
    const vec2 ODIM = vec2(this->output_w, this->output_h);

    //render bg
    bgObj.render(this->g, this->ui_proj, ODIM);

    //render wins
    for (UIWin *u : this->c_wins) {
        if (!u) return;

        u->render(this->g, this->ui_proj, ODIM);
    }
}

i64 UIInst::__int_addUWin(UIWin *win) {
    if (!win) return -1;

    size_t loc = this->c_wins.size();
    this->c_wins.push_back(win);

    return (signed) loc;
}

void UIInst::__int_notifWinDestroyed(size_t loc) {
    if (loc > this->c_wins.size())
        return;

    this->c_wins[loc] = nullptr;
}