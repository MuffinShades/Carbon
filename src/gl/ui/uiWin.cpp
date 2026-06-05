#include "uiWin.hpp"
#include "uiInst.hpp"

void UIWin::move(vec4 dim) {
    this->dim = dim;
}

UIWin::UIWin() {
    this->a_inst = nullptr;
    this->inst_loc = -1;
}

UIWin* UIWin::createWin(UIInst *inst, vec4 dim) {
    UIWin* win = new UIWin();

    if (!win) {
        std::cout << "failed to create UIWin; bad malloc" << std::endl;
        return nullptr;
    }

    //ZeroMem(win, 1);

    win->dim = dim;
    win->bounds = dim;
    win->inst_loc = inst->__int_addUWin(win);
    win->a_inst = inst;

    win->ini();

    return win;
}

void UIWin::destroyWin(UIWin **win) {
    if (!win || !*win) return;

    //notify parent instance of the window being destroyed
    i64 LOC;

    if ((LOC = (*win)->inst_loc) >= 0 && (*win)->a_inst) {
        (*win)->a_inst->__int_notifWinDestroyed((size_t) LOC);
    }

    _safe_free_b(*win);

    *win = nullptr;
}

/*void UIWin::render(graphics *g, mat4 mmat) {
    if (!g) return;

    //TODO: render le win
};*/