#include "ui.hpp"
#include "../geometry/rect.hpp"

i32 UIObject::next_ui_age = 0;

RenderState *UIObject::def_object_rs = nullptr;
Shader UIObject::ui_simple_shader;

i32 UIObject::getNextAge() {
    return UIObject::next_ui_age++;
}

void UIObject::render(graphics *g, mat4 mmat, vec2 outputDim) {
    if (!g) {
        std::cout << "ui object error: invalid graphics handle!" << std::endl;
        return;
    }

    if (!g->IsGoodRenderState(def_object_rs)) {
        std::cout << "ui object error: did not load the ui library!" << std::endl;
        return;
    }

    g->SetRenderState(def_object_rs);
    g->Resize(outputDim.x, outputDim.y);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);

    SimpleUIVert bg_verts[6] = RECT_VERTS_EX(
        this->bounds.x,
        this->bounds.y,
        this->bounds.z,
        this->bounds.w,
        0.0f COMMA this->_DBG.dbgBgColor.r COMMA this->_DBG.dbgBgColor.g COMMA this->_DBG.dbgBgColor.b,
        0.0f COMMA this->_DBG.dbgBgColor.r COMMA this->_DBG.dbgBgColor.g COMMA this->_DBG.dbgBgColor.b,
        0.0f COMMA this->_DBG.dbgBgColor.r COMMA this->_DBG.dbgBgColor.g COMMA this->_DBG.dbgBgColor.b,
        0.0f COMMA this->_DBG.dbgBgColor.r COMMA this->_DBG.dbgBgColor.g COMMA this->_DBG.dbgBgColor.b
    );

    g->SetShader(&ui_simple_shader);

    ui_simple_shader.SetMat4("model_mat", &mmat);

    g->RenderBegin();
    g->PushVerts(bg_verts, 6, false);
    g->RenderFlush();
}

void UIObject::onFocus() {}
void UIObject::offFocus() {}

void UIObject::focus() {
    this->focused = true;
    this->onFocus();
}

void UIObject::unfocus() {
    this->focused = false;
    this->offFocus();
}

void UIObject::ini() {
    this->age = UIObject::getNextAge();
    this->preCompute();
}

void UIObject::onEvent(UIEvent ev) {

}

void UIObject::resize(UIScalarDimension new_bounds) {
    this->bounds = vec4(new_bounds.x, new_bounds.y, new_bounds.w, new_bounds.h);
}

void UIObject::preCompute() {
    //do nothing by default cause idk
    this->_DBG.dbgBgColor.color_mode = UIColor::RGB1;
    this->_DBG.dbgBgColor.r = 0.0f;
    this->_DBG.dbgBgColor.g = 0.5f;
    this->_DBG.dbgBgColor.b = 0.5f;
}

void UIObject::load() {
    if (!graphics::IsGoodRenderState(UIObject::def_object_rs)) {

        std::cout << "bad render state creating good one!" << std::endl;
        //load the default ui render state
        graphics tg;

        RenderStateDescriptor def_ui_desc;

        def_ui_desc.dynamic = true;

        def_object_rs = tg.CreateNewRenderState(def_ui_desc);

        std::cout << "rs ptr: " << (uintptr_t) def_object_rs << std::endl;

        tg.SetRenderState(def_object_rs);
        tg.VertexDefineBegin(sizeof(SimpleUIVert));
        tg.DefineVertexPart(0, vertexClassPart(SimpleUIVert, posf));
        tg.DefineVertexPart(1, vertexClassPart(SimpleUIVert, color));
        tg.VertexDefineEnd();

        tg.RestoreDefaultRenderState(); //just incase yk

        tg.free();
    }

    if (!ui_simple_shader.good()) {
        ui_simple_shader = Shader::LoadShaderFromFile(UI_SIMPLE_SHADER_VERT_PATH, UI_SIMPLE_SHADER_FRAG_PATH);
    }
}

void UIObject::close() {
    if (def_object_rs)
        graphics::DeleteRenderState(def_object_rs);
}

void UIObject::update() {}

//////////////Layout Stuff/////////////////   

void UILayout::addObj(UIObject obj) {

}

void UILayout::recomputeLayout() {

}