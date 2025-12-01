#include "polynom.hpp"

constexpr f32 one_third = 1.0f / 3.0f;
constexpr f32 i9 = 1.0f / 9.0f, i54 = 1.0f / 54.0f;

f32 cube_root_32(f32 v) {
    const f32 m = -(2.0f * (i32) (v < 0) - 1.0f);
    return m * powf(m * v, one_third);
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
            r = (-(27.0f * d) + b * (9.0f * c - 2.0f * bs)) * i54,
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

i32 solve_linear_32(f32 a, f32 b, f32 roots[1]) {
    roots[0] = -b/a;
    return 1;
}

i32 solve_re_quadratic_32(f32 a, f32 b, f32 c, f32 re_roots[2]) {
    if (a == 0)
        return solve_linear_32(b, c, re_roots);

    const f32 i = b*b - 4.0f*a*c;

    if (i < 0.0f)
        return 0;
    
    if (i == 0.0f) {
        re_roots[1] = -b / (2.0f*a);
        return 1;
    }

    const f32 r = sqrtf(i);
    const f32 i2a = 1.0f / (2.0f * a);

    re_roots[0] = (-b + r) * i2a;
    re_roots[1] = (-b - r) * i2a;

    return 2;
}

i32 solve_re_quadratic_64(f64 a, f64 b, f64 c, f64 re_roots[2]) {
    const f64 i = b*b - 4.0*a*c;

    if (i < 0.0)
        return 0;
    
    if (i == 0.0) {
        re_roots[1] = -b / (2*a);
        return 1;
    }

    const f64 r = sqrtf(i);
    const f64 i2a = 1.0 / (2.0 * a);

    re_roots[0] = (-b + r) * i2a;
    re_roots[1] = (-b - r) * i2a;

    return 2;
}

i32 solve_quadratic_32(f32 a, f32 b, f32 c) {
    return 0;
}

i32 solve_quadratic_64(f64 a, f64 b, f64 c) {
    return 0;
}

i32 solve_re_cubic_32_b(f32 b, f32 c, f32 d, f32 re_roots[3]) {
    if (!re_roots)
        return 0;

    f32 bs = b * b, 
            q = (3.0f * c - bs) * i9,
            r = (-(27.0f * d) + b * (9.0f * c - 2.0f * bs)) * i54,
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

i32 solve_re_cubic_64_a(f64 a, f64 b, f64 c, f64 d, f64 re_roots[3]) {
    return 0;
}

i32 solve_re_cubic_64_b(f64 b, f64 c, f64 d, f64 re_roots[3]) {
    return 0;
}