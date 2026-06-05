#include "colorpicker.hpp"

_UIStock _ColorPicker_Stock::stock;

Shader *_ColorPicker_Stock::getShader() {
    return stock.getShader();
}

void ColorPicker::preCompute() {
    

    //this->gs.s = _ColorPicker_Stock::getShader();

    //g->useGraphicsState(&this->gs);
   // g->iniStaticGraphicsState();

    //define the vertex's for the color picker
    //g->vertexStructureDefineBegin(sizeof(ColorPick_Vertex));
    //g->defineVertexPart(0, vertexClassPart(ColorPick_Vertex, pos));
    //g->defineVertexPart(1, vertexClassPart(ColorPick_Vertex, color));
    //g->vertexStructureDefineEnd();

    //s/v die
    /*ColorPick_Vertex vtx[] = RECT_VERTS_EX(
        0, 0, bounds.w, bounds.h, 
        1 COMMA 1 COMMA 1, 1 COMMA 0 COMMA 0, 0 COMMA 0 COMMA 0, 0 COMMA 0 COMMA 0
    );*/


   //g->useDefaultGraphicsState();
}

void ColorPicker::render(graphics *g, mat4 mmat, vec2 outputDim) {
    if (!g)
        return;

    //g->useGraphicsState(&this->gs);

    //g->render_no_geo_update();
    //g->useDefaultGraphicsState();
}