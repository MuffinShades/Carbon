#include "msutil.hpp"

/*

Polynomial Solvers

Source of 

*/

extern i32 solve_re_quadratic_32(f32 a, f32 b, f32 c);
extern i32 solve_re_quadratic_64(f32 a, f32 b, f32 c);

extern i32 solve_quadratic_32(f32 a, f32 b, f32 c);
extern i32 solve_quadratic_64(f32 a, f32 b, f32 c);

//real only
extern i32 solve_re_cubic_32_a(f32 a, f32 b, f32 c, f32 d);
extern i32 solve_re_cubic_32_b(f32 b, f32 c, f32 d);
extern i32 solve_re_cubic_64_a(f32 a, f32 b, f32 c, f32 d);
extern i32 solve_re_cubic_64_b(f32 b, f32 c, f32 d);

//real and imaginary
extern i32 solve_cubic_32_a(f32 a, f32 b, f32 c, f32 d);
extern i32 solve_cubic_32_b(f32 b, f32 c, f32 d);
extern i32 solve_cubic_64_a(f32 a, f32 b, f32 c, f32 d);
extern i32 solve_cubic_64_b(f32 b, f32 c, f32 d);