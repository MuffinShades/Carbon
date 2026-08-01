//contains all the basic ui objects that dont need much code
#include "ui.hpp"
#include "uiWin.hpp"
#include "../../ttf_render.hpp"
#include "../../content.hpp"

enum UIFontWeight {
    Thin = 100,
    ExtraLight = 200,
    Light = 300,
    Normal = 400,
    Medium = 500,
    SemiBold = 600,
    Bold = 700,
    ExtraBold = 800,
    Black = 900
};

struct UITextStyle {
    FontInst *font = nullptr;
    i32 font_size = 40;
    Color font_color = Color(255, 255, 255);
    bool bold = false, italic = false, underline = false, strikethrough = false;
    i32 font_weight = (i32) UIFontWeight::Normal;
};

class UIText : public UIObject {
private:
    static FontInst defFont;
    std::string txt = "";
    UITextStyle textStyle;

    static Shader texShader;
    GenericFontProperties translatedProp;

    const char *cstr = nullptr;
    
    void _translateFontProps();
public:
    UIText(){}

    static UIText *createNew(std::string str);

    void render(graphics *g, mat4 mmat, vec2 outputDim) override;

    void setText(std::string str);
    void setFont(FontInst *font);
    std::string getText();
};

class UIImage : public UIObject {
private:
    BindableTexture iTex;
    static Shader genericImgDispShader;
public:
    UIImage(){}

    static UIImage *createNew(ContentSrc src);

    void render(graphics *g, mat4 mmat, vec2 outputDim) override;
    void setImg(ContentSrc src);
};