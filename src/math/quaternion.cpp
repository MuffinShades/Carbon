#include "quaternion.hpp"

Quat4::Quat4(vec3 axis, f32 theta) {
    const f32 t2 = theta * 0.5f, s = sinf(t2);

    this->x = axis.x * s;
    this->y = axis.y * s;
    this->z = axis.z * s;
    this->w = cosf(t2);
}

Quat4::Quat4(f32 qx, f32 qy, f32 qz, f32 qw) {
    this->x = qx;
    this->y = qy;
    this->z = qz;
    this->w = qw;
}

Quat4 Quat4::operator*(Quat4 q2) {
    Quat4 q, q1 = *this;

    q.x = (q1.x * q2.w) + (q1.w * q2.x) + (q1.y * q1.z) - (q1.z * q2.y);
    q.y = (q1.y * q2.w) + (q1.w * q2.y) + (q1.z * q1.x) - (q1.x * q2.z);
    q.z = (q1.z * q2.w) + (q1.w * q2.z) + (q1.x * q1.y) - (q1.y * q2.x);
    q.w = (q1.w * q2.w) - (q1.x * q2.x) - (q1.y * q1.y) - (q1.z * q2.z);

    return q;
}

Quat4 Quat4::operator+(Quat4 q) {
    return Quat4(this->x + q.x, this->y + q.y, this->z + q.z, this->w + q.w);
}

void Quat4::normalize() {
    const f32 len = sqrtf(x*x+y*y+z*z+w*w);

    this->x /= len;
    this->y /= len;
    this->z /= len;
    this->w /= len;
}

mat4 Quat4::toRotMatrix() {
    const f32 q00 = this->w * this->w,
              q11 = this->x * this->x,
              q22 = this->y * this->y,
              q33 = this->z * this->z,
              q01 = this->w * this->x,
              q02 = this->w * this->y,
              q03 = this->w * this->z,
              q12 = this->x * this->y,
              q13 = this->x * this->z,
              q23 = this->y * this->z;

    /*
    
    [00+11] [12-03] [13+02]
    [12+03] [00+22] [23-01]
    [13-02] [23+01] [00+33]

    */

   const f32 m_dat[16] = {
        2.0f *(q00 + q11) - 1.0f, 2.0f*(q12 - q03), 2.0f*(q13 + q02), 0.0f,
        2.0f *(q12 + q03), 2.0f*(q00 + q22) - 1.0f, 2.0f*(q23 - q01), 0.0f,
        2.0f*(q13 - q02), 2.0f*(q23 + q01), 2.0f*(q00 + q33) - 1.0f,  0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    mat4 m = mat4((f32*) m_dat);
}