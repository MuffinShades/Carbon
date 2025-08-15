#include "vec.hpp"

vec2 vec2::operator+(vec2 b) {
    return vec2(this->x + b.x, this->y + b.y);
}

vec2 vec2::operator-(vec2 b) {
    return vec2(this->x - b.x, this->y - b.y);
}

vec2 vec2::operator*(vec2 b) {
    return vec2(this->x * b.x, this->y * b.y);
}

vec2 vec2::operator*(float s) {
    return vec2(this->x * s, this->y * s);
}

vec2 vec2::operator/(float s) {
    if (s == 0.0f)
        return *this;

    return vec2(this->x / s, this->y / s);
}

vec2 vec2::operator/(vec2 b) {
    if (b.x == 0.0f || b.y == 0.0f)
        b = vec2(1.0f, 1.0f);

    return vec2(this->x / b.x, this->y / b.y);
}

vec2 vec2::operator+(ivec2 b) {
    return vec2(this->x + b.x, this->y + b.y);
}

vec2 vec2::operator-(ivec2 b) {
    return vec2(this->x - b.x, this->y - b.y);
}

vec2 vec2::operator*(ivec2 b) {
    return vec2(this->x * b.x, this->y * b.y);
}

vec2 vec2::operator/(ivec2 b) {
    if (b.x == 0.0f || b.y == 0.0f)
        return *this;

    return vec2(this->x / b.x, this->y / b.y);
}

vec2 vec2::operator+(uvec2 b) {
    return vec2(this->x + b.x, this->y + b.y);
}

vec2 vec2::operator-(uvec2 b) {
    return vec2(this->x - b.x, this->y - b.y);
}

vec2 vec2::operator*(uvec2 b) {
    return vec2(this->x * b.x, this->y * b.y);
}

vec2 vec2::operator/(uvec2 b) {
    if (b.x == 0.0f || b.y == 0.0f)
        return *this;

    return vec2(this->x / b.x, this->y / b.y);
}

float vec2::DotProd(vec2 a, vec2 b) {
    return a.x * b.x + a.y * b.y;
}

float vec2::CrossProd(vec2 a, vec2 b) {
    return a.x * b.y - a.y * b.x;
}

float vec2::lenSqr() {
    return this->x*this->x + this->y*this->y;
}

float vec2::len() {
    return sqrtf(this->lenSqr());
}

void vec2::Normalize() {
    const float length = this->len();

    if (length != 0.0f) {
        this->x /= length;
        this->y /= length;
    }
}

vec2 vec2::GetNormal() {
    const float length = this->len();

    if (length != 0.0f)
        return vec2(this->x / length, this->y / length);
    else
        return *this;
}

vec3 vec3::operator+(vec3 b) {
    return vec3(this->x + b.x, this->y + b.y, this->z + b.z);
}

vec3 vec3::operator-(vec3 b) {
    return vec3(this->x - b.x, this->y - b.y, this->z - b.z);
}

vec3 vec3::operator*(vec3 b) {
    return vec3(this->x * b.x, this->y * b.y, this->z * b.z);
}

vec3 vec3::operator*(float s) {
    return vec3(this->x * s, this->y * s, this->z * s);
}

vec3 vec3::operator/(float s) {
    if (s == 0.0f)
        return *this;

    return vec3(this->x / s, this->y / s, this->z / s);
}

vec3 vec3::operator/(vec3 b) {
    if (b.x == 0.0f || b.y == 0.0f || b.z == 0.0f)
        b = vec3(1.0f, 1.0f, 1.0f);

    return vec3(this->x / b.x, this->y / b.y, this->z / b.z);
}

vec3 vec3::operator+(ivec3 b) {
    return vec3(this->x + b.x, this->y + b.y, this->z + b.z);
}

vec3 vec3::operator-(ivec3 b) {
    return vec3(this->x - b.x, this->y - b.y, this->z - b.z);
}

vec3 vec3::operator*(ivec3 b) {
    return vec3(this->x * b.x, this->y * b.y, this->z * b.z);
}

vec3 vec3::operator/(ivec3 b) {
    if (b.x == 0.0f || b.y == 0.0f || b.z == 0.0f)
        b = ivec3(1, 1, 1);

    return vec3(this->x / b.x, this->y / b.y, this->z / b.z);
}

vec3 vec3::operator+(uvec3 b) {
    return vec3(this->x + b.x, this->y + b.y, this->z + b.z);
}

vec3 vec3::operator-(uvec3 b) {
    return vec3(this->x - b.x, this->y - b.y, this->z - b.z);
}

vec3 vec3::operator*(uvec3 b) {
    return vec3(this->x * b.x, this->y * b.y, this->z * b.z);
}

vec3 vec3::operator/(uvec3 b) {
    if (b.x == 0.0f || b.y == 0.0f || b.z == 0.0f)
        b = uvec3(1, 1, 1);

    return vec3(this->x / b.x, this->y / b.y, this->z / b.z);
}


float vec3::DotProd(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 vec3::CrossProd(vec3 a, vec3 b) {
    return vec3(
        a.y * b.z - a.z * b.y, 
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

float vec3::lenSqr() {
    return this->x * this->x + this->y * this->y + this->z * this->z;
}

float vec3::len() {
    return sqrtf(this->lenSqr());
}

vec3 vec3::Normalize(vec3 v) {
    const f32 length = v.len();

    if (length != 0.0f) {
        v.x /= length;
        v.y /= length;
        v.z /= length;
    }

    return v;
}

vec3 vec3::GetNormal() {
    const float length = this->len();

    if (length != 0.0f)
        return vec3(this->x / length, this->y / length, this->z / length);
    else
        return *this;
}


vec4 vec4::operator+(vec4 b) {
    return vec4(this->x + b.x, this->y + b.y, this->z + b.z, this->w + b.w);
}

vec4 vec4::operator-(vec4 b) {
    return vec4(this->x - b.x, this->y - b.y, this->z - b.z, this->w - b.w);
}

vec4 vec4::operator*(vec4 b) {
    return vec4(this->x * b.x, this->y * b.y, this->z * b.z, this->w * b.w);
}

vec4 vec4::operator*(float s) {
    return vec4(this->x * s, this->y * s, this->z * s, this->w * s);
}

vec4 vec4::operator/(float s) {
    if (s == 0.0f)
        return *this;

    return vec4(this->x / s, this->y / s, this->z / s, this->w / s);
}

vec4 vec4::operator/(vec4 b) {
    if (b.x == 0.0f || b.y == 0.0f || b.z == 0.0f)
        b = vec4(1.0f, 1.0, 1.0f, 1.0f);

    return vec4(this->x / b.x, this->y / b.y, this->z / b.z, this->w / b.w);
}

float vec4::DotProd(vec4 a, vec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float vec4::lenSqr() {
    return this->x * this->x + this->y * this->y + this->z * this->z + this->w * this->w;
}

float vec4::len() {
    return sqrt(this->lenSqr());
}

vec4 vec4::Normalize(vec4 v) {
    const float length = v.len();

    if (length != 0.0f) {
        v.x /= length;
        v.y /= length;
        v.z /= length;
        v.w /= length;
    }

    return v;
}

vec4 vec4::GetNormal() {
    const float length = this->len();

    if (length != 0.0f)
        return vec4(this->x / length, this->y / length, this->z / length, this->w / length);
    else
        return *this;
}

//ivec2 op ivec2
ivec2 ivec2::operator+(ivec2 b) {
    return ivec2(this->x + b.x, this->y + b.y);
}

ivec2 ivec2::operator-(ivec2 b) {
    return ivec2(this->x - b.x, this->y - b.y);
}

ivec2 ivec2::operator*(ivec2 b) {
    return ivec2(this->x * b.x, this->y * b.y);
}

ivec2 ivec2::operator/(ivec2 b) {
    if (b.x == 0 || b.y == 0)
        return *this;

    return ivec2(this->x / b.x, this->y / b.y);
}

//ivec2 op vec2
ivec2 ivec2::operator+(vec2 b) {
    return ivec2(this->x + b.x, this->y + b.y);
}

ivec2 ivec2::operator-(vec2 b) {
    return ivec2(this->x - b.x, this->y - b.y);
}

ivec2 ivec2::operator*(vec2 b) {
    return ivec2(this->x * b.x, this->y * b.y);
}

ivec2 ivec2::operator/(vec2 b) {
    if (b.x == 0 || b.y == 0)
        return *this;

    return ivec2(this->x / b.x, this->y / b.y);
}

//ivec2 op uvec2
ivec2 ivec2::operator+(uvec2 b) {
    return ivec2(this->x + b.x, this->y + b.y);
}

ivec2 ivec2::operator-(uvec2 b) {
    return ivec2(this->x - b.x, this->y - b.y);
}

ivec2 ivec2::operator*(uvec2 b) {
    return ivec2(this->x * b.x, this->y * b.y);
}

ivec2 ivec2::operator/(uvec2 b) {
    if (b.x == 0 || b.y == 0)
        return *this;

    return ivec2(this->x / b.x, this->y / b.y);
}

//--------------------------------------------------

uvec2 uvec2::operator+(ivec2 b) {
    return uvec2(this->x + b.x, this->y + b.y);
}

uvec2 uvec2::operator-(ivec2 b) {
    return uvec2(this->x - b.x, this->y - b.y);
}

uvec2 uvec2::operator*(ivec2 b) {
    return uvec2(this->x * b.x, this->y * b.y);
}

uvec2 uvec2::operator/(ivec2 b) {
    if (b.x == 0 || b.y == 0)
        return *this;

    return uvec2(this->x / b.x, this->y / b.y);
}

/*********************************/

uvec2 uvec2::operator+(vec2 b) {
    return uvec2(this->x + b.x, this->y + b.y);
}

uvec2 uvec2::operator-(vec2 b) {
    return uvec2(this->x - b.x, this->y - b.y);
}

uvec2 uvec2::operator*(vec2 b) {
    return uvec2(this->x * b.x, this->y * b.y);
}

uvec2 uvec2::operator/(vec2 b) {
    if (b.x == 0 || b.y == 0)
        return *this;

    return uvec2(this->x / b.x, this->y / b.y);
}

/*********************************/

uvec2 uvec2::operator+(uvec2 b) {
    return uvec2(this->x + b.x, this->y + b.y);
}

uvec2 uvec2::operator-(uvec2 b) {
    return uvec2(this->x - b.x, this->y - b.y);
}

uvec2 uvec2::operator*(uvec2 b) {
    return uvec2(this->x * b.x, this->y * b.y);
}

uvec2 uvec2::operator/(uvec2 b) {
    if (b.x == 0 || b.y == 0)
        return *this;

    return uvec2(this->x / b.x, this->y / b.y);
}

//--------------------------------------------------

ivec3 ivec3::operator+(ivec3 b) {
    return ivec3(this->x + b.x, this->y + b.y, this->z + b.z);
}

ivec3 ivec3::operator-(ivec3 b) {
    return ivec3(this->x - b.x, this->y - b.y, this->z - b.z);
}

ivec3 ivec3::operator*(ivec3 b) {
    return ivec3(this->x * b.x, this->y * b.y, this->z * b.z);
}

ivec3 ivec3::operator/(ivec3 b) {
    if (b.x == 0 || b.y == 0 || b.z == 0)
        return *this;
    return ivec3(this->x / b.x, this->y / b.y, this->z / b.z);
}

/*********************************/

ivec3 ivec3::operator+(vec3 b) {
    return ivec3(this->x + b.x, this->y + b.y, this->z + b.z);
}

ivec3 ivec3::operator-(vec3 b) {
    return ivec3(this->x - b.x, this->y - b.y, this->z - b.z);
}

ivec3 ivec3::operator*(vec3 b) {
    return ivec3(this->x * b.x, this->y * b.y, this->z * b.z);
}

ivec3 ivec3::operator/(vec3 b) {
    if (b.x == 0 || b.y == 0 || b.z == 0)
        return *this;
    return ivec3(this->x / b.x, this->y / b.y, this->z / b.z);
}

/*********************************/

ivec3 ivec3::operator+(uvec3 b) {
    return ivec3(this->x + b.x, this->y + b.y, this->z + b.z);
}

ivec3 ivec3::operator-(uvec3 b) {
    return ivec3(this->x - b.x, this->y - b.y, this->z - b.z);
}

ivec3 ivec3::operator*(uvec3 b) {
    return ivec3(this->x * b.x, this->y * b.y, this->z * b.z);
}

ivec3 ivec3::operator/(uvec3 b) {
    if (b.x == 0 || b.y == 0 || b.z == 0)
        return *this;
    return ivec3(this->x / b.x, this->y / b.y, this->z / b.z);
}

//--------------------------------------------------

uvec3 uvec3::operator+(ivec3 b) {
    return uvec3(this->x + b.x, this->y + b.y, this->z + b.z);
}

uvec3 uvec3::operator-(ivec3 b) {
    return uvec3(this->x - b.x, this->y - b.y, this->z - b.z);
}

uvec3 uvec3::operator*(ivec3 b) {
    return uvec3(this->x * b.x, this->y * b.y, this->z * b.z);
}

uvec3 uvec3::operator/(ivec3 b) {
    if (b.x == 0 || b.y == 0 || b.z == 0)
        return *this;
    return uvec3(this->x / b.x, this->y / b.y, this->z / b.z);
}

/*********************************/

uvec3 uvec3::operator+(vec3 b) {
    return uvec3(this->x + b.x, this->y + b.y, this->z + b.z);
}

uvec3 uvec3::operator-(vec3 b) {
    return uvec3(this->x - b.x, this->y - b.y, this->z - b.z);
}

uvec3 uvec3::operator*(vec3 b) {
    return uvec3(this->x * b.x, this->y * b.y, this->z * b.z);
}

uvec3 uvec3::operator/(vec3 b) {
    if (b.x == 0 || b.y == 0 || b.z == 0)
        return *this;
    return uvec3(this->x / b.x, this->y / b.y, this->z / b.z);
}

/*********************************/

uvec3 uvec3::operator+(uvec3 b) {
    return uvec3(this->x + b.x, this->y + b.y, this->z + b.z);
}

uvec3 uvec3::operator-(uvec3 b) {
    return uvec3(this->x - b.x, this->y - b.y, this->z - b.z);
}

uvec3 uvec3::operator*(uvec3 b) {
    return uvec3(this->x * b.x, this->y * b.y, this->z * b.z);
}

uvec3 uvec3::operator/(uvec3 b) {
    if (b.x == 0 || b.y == 0 || b.z == 0)
        return *this;
    return uvec3(this->x / b.x, this->y / b.y, this->z / b.z);
}