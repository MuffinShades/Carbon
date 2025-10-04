#include <iostream>
#include "../mat.hpp"
#include "../vec.hpp"

class Quat4 {
public:
    f32 x = 0.0f,y = 0.0f,z = 0.0f,w = 1.0f;    

    Quat4(vec3 axis, f32 theta);
    Quat4(f32 qx, f32 qy, f32 qz, f32 qw = 1.0f);
    Quat4(){};
    Quat4 operator*(Quat4 q2);
    Quat4 operator+(Quat4 q);
    mat4 toRotMatrix();

    void normalize();
};