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
	//in_memcpy(this->m, dat, sizeof(f32) * MAT4_MEMALLOC);

	forrange(MAT4_MEMALLOC)
		this->m[i] = dat[i];
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
			r.m[MAT4_POINT(j,i)] = m1[0][j] * m2[i][0] + m1[1][j] * m2[i][1] + m1[2][j] * m2[i][2] + m1[3][j] * m2[i][3];

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
	//constexpr f64 rad_adjust = 3.1415926 / 180.0;

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
	const f32 cotan = 1.0f / tanf(mu_rad(fov * 0.5f));
	const f32 right = fNear * (1.0f / cotan), top = right / aspectRatio;
	const f32 plane_diff_rec = 1.0f / (fNear - fFar);

	f32 dat[16] = {
		cotan / aspectRatio, 0.0f, 0.0f, 0.0f,
		0.0f, cotan, 0.0f, 0.0f,
		0.0f, 0.0f, (fFar + fNear) * plane_diff_rec, -1.0f,
		0.0f, 0.0f, ((2.0f * fFar * fNear) * plane_diff_rec), 0.0f
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
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		pos.x, pos.y, pos.z, 1.0f
	};

	//std::cout << "Translation dat:" << std::endl;
	//forrange(16) std::cout << dat[i] << " ";
	//std::cout << std::endl;

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

		tmp.z * a.x + Sin * a.y,
		tmp.z * a.y - Sin * a.x,
		Cos + tmp.z * a.z,
		0.0f,

		0.0f, 0.0f, 0.0f, 1.0f
	};

	return m * mat4(rMat);
}

mat4 mat4::LookAt(/*vec3 right, vec3 up, vec3 dir*/ vec3 pos, vec3 tar, vec3 up) {
	const vec3 f = vec3::Normalize(tar - pos);
	const vec3 r = vec3::Normalize(vec3::CrossProd(f, up));
	const vec3 u = vec3::CrossProd(r, f);

	f32 dat[16] = {
		r.x, u.x, -f.x, 0.0f,
		r.y, u.y, -f.y, 0.0f,
		r.z, u.z, -f.z, 0.0f,
		//--------------------------//
		-vec3::DotProd(r, pos),
		-vec3::DotProd(u, pos),
		 vec3::DotProd(f, pos),
		1.0f
	};

	/*up = vec3::Normalize(up);
	vec3 f = vec3::Normalize(tar - pos);
	vec3 r = vec3::CrossProd(f, up);
	vec3 u = vec3::CrossProd(f, r);

	u = vec3::Normalize(u);
	r = vec3::Normalize(r);
	r.y = -r.y;

	vec3 t = {
		vec3::DotProd(pos, r),
		vec3::DotProd(pos, u),
		vec3::DotProd(pos, f)
	};

	f32 dat[16] = {
		r.x, u.x, -f.x, 0.0f,
		r.y, u.y, -f.y, 0.0f,
		r.z, u.z, -f.z, 0.0f,
		-t.x, -t.y, -t.z, 1.0f
	};*/

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

mat4 mat4::CreateCrossMatrix(vec3 v) {
	f32 dat[16] = {
		0.0f, -v.z, v.y, 0.0f,
		v.z, 0.0f, -v.x, 0.0f,
		-v.y, v.x, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	return mat4(dat);
}

mat4 mat4::Cross(mat4 m, vec3 v) {
	f32 cMat[16] = {
		0.0f, -v.z, v.y, 0.0f,
		v.z, 0.0f, -v.x, 0.0f,
		-v.y, v.x, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	return m * mat4(cMat);
}

mat4 mat4::operator*(f32 s) {
	f32 nm[16];

	forrange(16)
		nm[i] = m[i] * s;

	return mat4(nm);
}

mat4 mat4::operator+(mat4 m2) {
	f32 nm[16];

	forrange(16)
		nm[i] = m[i] + m2.m[i];

	return mat4(nm);
}