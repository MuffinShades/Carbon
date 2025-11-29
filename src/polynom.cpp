#include "polynom.hpp"

constexpr f32 one_third = 1.0f / 3.0f;
constexpr f32 i9 = 1.0f / 9.0f, i54 = 1.0f / 54.0f;

f32 cube_root_32(f32 v) {
    const bool cmp = v < 0;
    return (-1.0f * cmp) * powf(-1.0f * cmp * v, one_third);
}

i32 solve_re_cubic_32_a(f32 a, f32 b, f32 c, f32 d, f32 re_roots[3]) {
    if (!re_roots)
        return 0;

    if (a == 0.0f) {
        return solve_re_quadratic_32(b, c, d, re_roots);
    }

    const f32 ia = 1.0f / a;

    b *= ia; c *= ia; d *= ia;

    f32 bs = b * b, 
            q = (3.0f * c - bs) * i9,
            r = -(27.0f * d) + b * (9.0f * c - 2.0f * bs) * i54,
            q3 = q*q*q,
            dis = q3 + r*r;

    const f32 xi = (b * one_third);

    if (dis > 0) {
        const f32 root_dis = sqrtf(dis);

        f32 s = cube_root_32(r + root_dis);
        f32 t = cube_root_32(r - root_dis);

        re_roots[0] = -xi + s + t;
        re_roots[1] = 0;
        re_roots[2] = 0;

        return 1;
    }

    if (dis == 0) {
        const f32 crt_r = cube_root_32(r);
        re_roots[0] = -xi + 2.0f * crt_r;
        re_roots[2] = (re_roots[1] = (
            -(crt_r + xi)
        ));
        return 2;
    }

    q *= -1.0f;

    const f32 eta = acosf(r / sqrtf(-q3)) * one_third;
    const f32 r13 = 2.0f * sqrt(q);

    const f32 rot_1 = 2.0f * mu_pi * one_third,
              rot_2 = 4.0f * mu_pi * one_third;

    re_roots[0] = -xi + r13*cosf(eta + 0);
    re_roots[1] = -xi + r13*cosf(eta + rot_1);
    re_roots[2] = -xi + r13*cosf(eta + rot_2);

    return 3;
}