#include "uiBasicObjects.hpp"
#include "../graphics.hpp"
#include "../geometry/rect.hpp"

#define BASICIMGDISP_SHADER_VERT ""
#define BASICIMGDISP_SHADER_FRAG ""

#define DEFFONT_SRC "C:\\Windows\\Fonts\\arial.ttf"

//Shader UIText::texShader;

FontInst UIText::defFont;

void UIText::_translateFontProps() {
    this->translatedProp.scale.pt = this->textStyle.font_size;
    this->translatedProp.style.italic = this->textStyle.italic;
    this->translatedProp.style.color = this->textStyle.font_color;
}

void UIText::render(graphics *g, mat4 mmat, vec2 outputDim) {
    if (!g)
        return;

    if (!defFont.good) {
        defFont = ttfRender::GenerateUnicodeMSDFSubset(DEFFONT_SRC, UnicodeRange::SimpleAlphabet, sdf_width_dim(32), true);
    }

    /*char defText[] = {'N','U','L','T','X','T','\0'};
    char *str = const_cast<char*>(this->cstr);

    if (!str)
        str = defText;*/
    char *str;
    str = const_cast<char*>(this->txt.c_str());

    FontInst *fnt = &defFont;

    if (this->textStyle.font) {
        fnt = this->textStyle.font;
    }

    g->RenderString(fnt, this->bounds.x, this->bounds.y, 0.0f, str, this->translatedProp, true);
}

void UIText::setText(std::string str) {
    this->txt = str;
}

void UIText::setFont(FontInst *font) {
    if (!font)
        return;

    this->textStyle.font = font;
}

std::string UIText::getText() {
    return this->txt;
}

UIText *UIText::createNew(std::string str) {
    UIText _fill;
    UIText *text = UI::_allocObj<UIText>(&_fill);

    if (!text) {
        std::cout << "Failed to create text object. Make sure you called UI::load or something" << std::endl;
        return nullptr; 
    }

    text->setText(str);
    text->_translateFontProps();
    text->ini();

    return text;
}

Shader UIImage::genericImgDispShader;

void UIImage::render(graphics *g, mat4 mmat, vec2 outputDim) {
    if (!g) return;

    if (!genericImgDispShader.good()) {
        genericImgDispShader = Shader::LoadShaderFromFile(BASICIMGDISP_SHADER_VERT, BASICIMGDISP_SHADER_FRAG);
    }

    SimpleUIVert img_verts[6] = RECT_VERTS_EX(
        this->bounds.x,
        this->bounds.y,
        this->bounds.z,
        this->bounds.w,
        0.0f COMMA 0.0f COMMA 0.0f COMMA 0.0f,
        0.0f COMMA 0.0f COMMA 0.0f COMMA 0.0f,
        0.0f COMMA 0.0f COMMA 0.0f COMMA 0.0f,
        0.0f COMMA 0.0f COMMA 0.0f COMMA 0.0f
    );

    //TODO: bind the image to slot 1

    //very simply draw the image
    g->RestoreDefaultRenderState();
    g->SetShader(&genericImgDispShader);
    g->RenderBegin();
    g->PushVerts(img_verts, sizeof(img_verts) / sizeof(SimpleUIVert), true);
    g->RenderFlush();
}