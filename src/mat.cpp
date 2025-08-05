#include "mat.hpp"

void mat4::attemptFree() {
	
}

mat4::mat4() {
	ZeroMem(this->m, MAT4_MEMALLOC);
}

mat4::mat4(f32 *dat, bool free) {
	in_memcpy(this->m, dat, sizeof(f32) * MAT4_MEMALLOC);

	if (free)
		_safe_free_a(dat);
}

mat4::mat4(f32 dat[MAT4_MEMALLOC]) {
	in_memcpy(this->m, dat, sizeof(f32) * MAT4_MEMALLOC);
}

mat4::mat4(f32 value) {
	ZeroMem(this->m, MAT4_MEMALLOC);
	this->m[0] = value;
	this->m[5] = value;
	this->m[10] = value;
	this->m[15] = value;
}

const f32* mat4::glPtr() {
	return const_cast<const f32*>(this->m);
}

mat4 mat4::operator*(mat4 m2) {
	mat4 r;
	mat4 m1 = (*this);

    i32 i,j;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			r.m[MAT4_POINT(j,i)] = m1[j][0] * m2[0][i] + m1[j][1] * m2[1][i] + m1[j][2] * m2[2][i] + m1[j][3] * m2[3][i];

	return r;
}

vec3 mat4::MultiplyMat4Vec(vec3 v, mat4 m) {
	vec3 o;

	o.x = v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0] + m[3][0];
	o.y = v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1] + m[3][1];
	o.z = v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2] + m[3][2];

	float w = v.x * m[0][3] + v.y * m[1][3] + v.z * m[2][3] + m[3][3];


	if (w != 0)
	{
		o.x /= w;
		o.y /= w;
		o.z /= w;
	}

	return o;
}

vec3 mat4::operator*(vec3 v) {
    return mat4::MultiplyMat4Vec(v, *this);
}

mat4 mat4::CreatePersepctiveProjectionMatrix(float fov, float aspectRatio, float fNear, float fFar) {
	constexpr f64 rad_adjust = 3.1415926 / 180.0;

	/*f32 fovRad = 1.0f / tanf((fov * 0.5f) * rad_adjust);

	f32 dat[16] = {
		fovRad / aspectRatio, 0.0f, 0.0f, 0.0f,
		0.0f, fovRad, 0.0f, 0.0f,
		0.0f, 0.0f, (fFar + fNear) / (fNear - fFar), -1.0f,
		0.0f, 0.0f, ((2.0f * fFar * fNear) / (fNear - fFar)), 0.0f
	};*/

	//fovY
	/*const f32 tangent = tanf((fov * 0.5f) * rad_adjust);
	const f32 top = fNear * tangent, right = top * aspectRatio;
	const f32 plane_diff_rec = 1.0f / (fFar - fNear);

	f32 dat[16] = {
		fNear / right, 0.0f, 0.0f, 0.0f,
		0.0f, fNear / top, 0.0f, 0.0f,
		0.0f, 0.0f, -(fFar + fNear) * plane_diff_rec, -1.0f,
		0.0f, 0.0f, (-(2.0f * fFar * fNear) * plane_diff_rec), 0.0f
	};*/

	//fovX
	const f32 tangent = tanf((fov * 0.5f) * rad_adjust);
	const f32 right = fNear * tangent, top = right / aspectRatio;
	const f32 plane_diff_rec = 1.0f / (fFar - fNear);

	f32 dat[16] = {
		fNear / right, 0.0f, 0.0f, 0.0f,
		0.0f, fNear / top, 0.0f, 0.0f,
		0.0f, 0.0f, -(fFar + fNear) * plane_diff_rec, -1.0f,
		0.0f, 0.0f, (-(2.0f * fFar * fNear) * plane_diff_rec), 0.0f
	};

	return mat4(dat);
};

mat4 mat4::CreateRotationMatrixX(float theta, vec3 origin) {
	float _cos = cosf(theta), _sin = sinf(theta);

	float dat[16] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, _cos, -_sin, 0.0f,
		0.0f, _sin, _cos, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	};

	return mat4(dat);
};

mat4 mat4::CreateRotationMatrixY(float theta, vec3 origin) {
	float _cos = cosf(theta), _sin = sinf(theta);

	float dat[16] = {
		_cos, 0.0f, _sin, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		-_sin, 0.0f, _cos, 0.0f,
	    0.0f, 0.0f, 0.0f, 1.0f
	};

	return mat4(dat);
};

mat4 mat4::CreateRotationMatrixZ(float theta, vec3 origin) {
	float _cos = cosf(theta), _sin = sinf(theta);

	float dat[16] = {
		_cos, -_sin, 0.0f, 0.0f,
		_sin, _cos, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	return mat4(dat);
};

mat4 mat4::CreateTranslationMatrix(vec3 pos) {
	float dat[16] = {
		1.0f, 0.0f, 0.0f, pos.x,
		0.0f, 1.0f, 0.0f, pos.y,
		0.0f, 0.0f, 1.0f, pos.z,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	return mat4(dat);
}

mat4 mat4::CreateOrthoProjectionMatrix(float right, float left, float bottom, float top, float zNear, float zFar) {
	/*float* dat = new float[16];
	memset(dat, NULL, sizeof(float) * 16);
	dat[0] = 2.0f / (right - left);
	dat[5] = 2.0f / (top - bottom);
	dat[10] = -2.0f / (zFar - zNear);
	dat[12] = -((right + left) / (right - left));
	dat[13] = -((top + bottom) / (top - bottom));
	dat[14] = -((zFar + zNear) / (zFar - zNear));
	dat[15] = 1.0f;*/

	f32 dat[16] = {
		2.0f / (right - left), 0.0f, 0.0f, -((right + left) / (right - left)),
		0.0f, 2.0f / (top - bottom), 0.0f, -((top + bottom) / (top - bottom)),
		0.0f, 0.0f, -2.0f / (zFar - zNear), -((zFar + zNear) / (zFar - zNear)),
		0.0f, 0.0f, 0.0f, 1.0f
	};

	return mat4(dat);
};

mat4 mat4::Translate(mat4 m, vec3 pos) {
	return m * CreateTranslationMatrix(pos);
}

mat4 mat4::Rotate(mat4 m, float theta, vec3 axis) {
	vec3 a = vec3::Normalize(axis);
	float Cos = cosf(theta), Sin = sinf(theta), iCos = 1.0f - Cos;
	vec3 tmp = vec3(iCos * a.x, iCos * a.y, iCos * a.z);

	//calculation help from glm
	float rMat[] = {
		Cos + tmp.x * a.x,
		tmp.x * a.y + Sin * a.z,
		tmp.x * a.z - Sin * a.y,
		0.0f,

		tmp.y * a.x - Sin * a.z,
		Cos + tmp.y * a.y,
		tmp.y * a.z + Sin * a.x,
		0.0f,

		tmp.z * axis.x + Sin * axis.y,
		tmp.z * axis.y - Sin * axis.x,
		Cos + tmp.z * axis.z,
		0.0f,

		0.0f, 0.0f, 0.0f, 1.0f
	};

	return m * mat4(rMat);
}

mat4 mat4::LookAt(vec3 right, vec3 up, vec3 dir) {
	vec3 f = vec3::Normalize(up - right);
	vec3 s = vec3::Normalize(vec3::CrossProd(f, dir));
	vec3 u = vec3::CrossProd(s,f);

	f32 dat[16] = {
		s.x, u.x, -f.x, 0.0f,
		s.y, u.y, -f.y, 0.0f,
		s.z, u.z, -f.z, 0.0f,

		-vec3::DotProd(s, right),
		-vec3::DotProd(u, right),
		 vec3::DotProd(f, right),
		1.0f
	};

	return mat4(dat);
}

f32 mat4::_Mat4Row::operator[](size_t column) {
	if (dat)
		return dat[(row << 2) + column];

	return 0.0f;
}

mat4::_Mat4Row mat4::operator[](i32 row) {
	return _Mat4Row(row, this->m);
}