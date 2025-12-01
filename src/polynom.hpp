#include "msutil.hpp"

/*

Polynomial Solvers

Source of 

*/

extern i32 solve_linear_32(f32 a, f32 b, f32 roots[1]);

extern i32 solve_re_quadratic_32(f32 a, f32 b, f32 c, f32 re_roots[2]);
extern i32 solve_re_quadratic_64(f64 a, f64 b, f64 c, f64 re_roots[2]);

extern i32 solve_quadratic_32(f32 a, f32 b, f32 c);
extern i32 solve_quadratic_64(f64 a, f64 b, f64 c);

//real only
extern i32 solve_re_cubic_32_a(f32 a, f32 b, f32 c, f32 d, f32 re_roots[3]);
extern i32 solve_re_cubic_32_b(f32 b, f32 c, f32 d, f32 re_roots[3]);
extern i32 solve_re_cubic_64_a(f64 a, f64 b, f64 c, f64 d, f64 re_roots[3]);
extern i32 solve_re_cubic_64_b(f64 b, f64 c, f64 d, f64 re_roots[3]);

//real and imaginary
extern i32 solve_cubic_32_a(f32 a, f32 b, f32 c, f32 d);
extern i32 solve_cubic_32_b(f32 b, f32 c, f32 d);
extern i32 solve_cubic_64_a(f32 a, f32 b, f32 c, f32 d);
extern i32 solve_cubic_64_b(f32 b, f32 c, f32 d);