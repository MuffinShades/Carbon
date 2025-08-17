#include <iostream>
#include "graphics.hpp"
#include "../mat.hpp"

struct LightSource {
    vec3 color;
    f32 intensity;
    vec3 pos;
};

class ShadowBuffer : FrameBuffer {
protected:
    mat4 lightMatrix, lightView;
    static Shader shadowShader;
public:
    ShadowBuffer(LightSource l, u32 w, u32 h, vec2 zRange) : FrameBuffer(FrameBuffer::Depth, w, h) {
        this->lightMatrix = mat4::CreateOrthoProjectionMatrix(w, 0.0f, h, 0.0f, zRange.x, zRange.y);

        /*const vec3 globalCenter = vec3(0.0f, 0.0f, 0.0f),
             globalUp = vec3(0.0f, 1.0f, 0.0f),
             front = vec3::Normalize(l.pos),
             r = vec3::CrossProd(front, globalUp),
             lu = vec3::CrossProd(r, front);

        this->lightView = mat4::LookAt(r, lu, front);*/

        this->lightView = mat4::LookAt(l.pos, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
        this->specialVal = _CARBONGL_SHADOW_SPECIAL_VAL; //
    }


};