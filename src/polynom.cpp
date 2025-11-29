#include "polynom.hpp"

i32 solve_re_cubic_32_a(f32 a, f32 b, f32 c, f32 d) {
    if (a == 0.0f) {

    }

    const f32 ia = 1.0f / a;

    b *= ia; c *= ia; d *= ia;

    constexpr f32 i9 = 1.0f / 9.0f, i54 = 1.0f / 54.0f;
    const
        f32 bs = b * b, 
            q = (3.0f * c - bs) * i9,
            r = -(27.0f * d) + b * (9.0f * c - 2.0f * bs) * i54,
            dis = q*q*q + r*r;

    if (dis > 0) {

    } else {
        
    }
}