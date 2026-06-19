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

    //create the core layout and bind to it
    win->core_layout = UILayout::createNewBasicLayout();
    win->_og_layout = win->core_layout;

    if (win->_og_layout) {
        win->_bindSelfToObj(win->_og_layout);
    } else {
        std::cout << "uiwin error: could not create core layout! (probably and allocation fail)" << std::endl;
    }

    //ini the win
    win->ini();

    return win;
}

//so I dont have to rewrite this code everytime I bind to one of the window's core objects
void UIWin::_bindSelfToObj(UIObject *obj) {
    if (!obj)
        return;

    _UIBindingInf bInf = {
        .inst = this->a_inst,
        .parent = this
    };

    obj->_ext_bind(bInf);
}

void UIWin::destroyWin(UIWin **win) {
    if (!win || !*win) return;

    UIWin *wwin = *win;

    if (wwin->_og_layout)
        UI::destroyUIObject((UIObject**)(&wwin->_og_layout));

    //notify parent instance of the window being destroyed
    i64 LOC;

    if ((LOC = wwin->inst_loc) >= 0 && wwin->a_inst) {
        wwin->a_inst->__int_notifWinDestroyed((size_t) LOC);
    }

    _safe_free_b(wwin);
    *win = nullptr;
}

void UIWin::addChild(UIObject *obj) {
    if (!this->core_layout || !obj) {
        std::cout << "warning could not add object to win: either invalid object or window core layout | layout: " << ((uintptr_t) this->core_layout) << " obj: " << ((uintptr_t) obj) << std::endl;
        return;
    }

    this->core_layout->addObj(obj);
}

void UIWin::render(graphics *g, mat4 mmat, vec2 outputDim) {
    if (!g) return;

    this->core_layout->render(g, mmat, outputDim);
}

/*void UIWin::render(graphics *g, mat4 mmat) {
    if (!g) return;

    //TODO: render le win
};*/