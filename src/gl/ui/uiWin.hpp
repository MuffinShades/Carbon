#pragma once
#include "ui.hpp"

/**
 * 
 * Top-level ui element that is directly connected to the window
 * 
 * This class contqins many important things such as the general
 * projection matrix for all the ui elements of a window and much
 * more
 * 
 */
class UIWin : public UIObject {
private:
    vec4 dim;
    i64 inst_loc = -1;
    class UIInst *a_inst = nullptr;

    std::vector<UIObject> children;
    UILayout core_layout;
public:
    UIWin();

    void move(vec4 dim);
    void addChild(UIObject obj);
    void setPrimaryLayout(UILayout lay);
    //void render(graphics *g, mat4 mmat) override;

    static UIWin* createWin(class UIInst *inst, vec4 dim);
    static void destroyWin(UIWin **win);

    friend class UIInst;
    friend UIWin* createWin(class UIInst *inst, vec4 dim);
};