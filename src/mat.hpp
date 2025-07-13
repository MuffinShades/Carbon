#pragma once
#include <iostream>
#include "vec.hpp"
#include <msutil.hpp>
#include <cmath>

#define MAT4_POINT(x,y) (x+y*4)
#define MAT4_MEMALLOC 16


class mat4 {
public:
	float *m = nullptr;

	void attemptFree() {
		std::cout << "Dealloc: " << this->m << std::endl;
		if (this->m != nullptr)
			delete[] this->m;
		this->m = nullptr;
	}

	//initializers
	mat4() {
		this->m = new float[MAT4_MEMALLOC];
		ZeroMem(this->m, MAT4_MEMALLOC);
	}

	mat4(float *dat, bool free) {
		this->m = new float[MAT4_MEMALLOC];
		memcpy(this->m, dat, sizeof(float) * MAT4_MEMALLOC);

		if (free)
			delete[] dat;
	}

	mat4(float dat[MAT4_MEMALLOC]) {
		this->attemptFree();

		this->m = new float[MAT4_MEMALLOC];
		memcpy(this->m, dat, sizeof(float) * MAT4_MEMALLOC);
	}

	/*
	Constructor for Identity matrix

	1 0 0 0
	0 1 0 0
	0 0 1 0
	0 0 0 1
	
	
	*/
	mat4(float value, bool free) {
		this->m = new float[MAT4_MEMALLOC];
		ZeroMem(this->m, MAT4_MEMALLOC);
		this->m[0] = value;
		this->m[5] = value;
		this->m[10] = value;
		this->m[15] = value;
	}

	float* glPtr() {
		return this->m;
	}

	//temp class for [][] see https://stackoverflow.com/questions/27718586/overloading-double-subscript-operator-in-c
	class _Mat4Row {
	private:
		float* dat;
		int row;
	public:
		_Mat4Row(int row, float dat[16]) : row(row) {
			this->dat = dat;
		}

		float operator[](int colum) {
			if (dat != nullptr) {
				return dat[row * 4 + colum];
			}

			return 0.0f;
		}
	};

	//operator []
	_Mat4Row operator[](int row) {
		return _Mat4Row(row, this->m);
	}

	mat4 operator*(mat4 m2) {
		mat4 r;
		mat4 m1 = (*this);

		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				r.m[MAT4_POINT(j,i)] = m1[j][0] * m2[0][i] + m1[j][1] * m2[1][i] + m1[j][2] * m2[2][i] + m1[j][3] * m2[3][i];

		return r;
	}


	//utility functions
	static vec3 MultiplyMat4Vec(vec3 v, mat4 m) {
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

    vec3 operator*(vec3 v) {
        return mat4::MultiplyMat4Vec(v, *this);
    }

	static mat4 *CreatePersepctiveProjectionMatrix(float fov, float aspectRatio, float fNear, float fFar) {
		float fovRad = 1.0f / tanf(fov * 0.5f / 180.0f * 3.1415926f);

		float dat[16] = {
			(1.0f/aspectRatio) * fovRad, 0.0f, 0.0f, 0.0f,
			0.0f, fovRad, 0.0f, 0.0f,
			0.0f, 0.0f, (fFar + fNear) / (fNear - fFar), -1.0f,
			0.0f, 0.0f, ((2 * fFar * fNear) / (fNear - fFar)), 0.0f
		};

		return new mat4(dat);
	};

	static mat4 *CreateRotationMatrixX(float theta, vec3 origin) {
		float _cos = cosf(theta), _sin = sinf(theta);

		float dat[16] = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, _cos, -_sin, 0.0f,
			0.0f, _sin, _cos, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f,
		};

		return new mat4(dat);
	};

	static mat4 *CreateRotationMatrixY(float theta, vec3 origin) {
		float _cos = cosf(theta), _sin = sinf(theta);

		float dat[16] = {
			_cos, 0.0f, _sin, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			-_sin, 0.0f, _cos, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};

		return new mat4(dat);
	};

	static mat4 *CreateRotationMatrixZ(float theta, vec3 origin) {
		float _cos = cosf(theta), _sin = sinf(theta);

		float dat[16] = {
			_cos, -_sin, 0.0f, 0.0f,
			_sin, _cos, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};

		return new mat4(dat);
	};

	static mat4 *CreateTranslationMatrix(vec3 pos) {
		float dat[16] = {
			1.0f, 0.0f, 0.0f, pos.x,
			0.0f, 1.0f, 0.0f, pos.y,
			0.0f, 0.0f, 1.0f, pos.z,
			0.0f, 0.0f, 0.0f, 1.0f
		};

		return new mat4(dat);
	}

	static mat4 *CreateOrthoProjectionMatrix(float right, float left, float bottom, float top, float zNear, float zFar) {
		float* dat = new float[16];
		memset(dat, NULL, sizeof(float) * 16);
		dat[0] = 2.0f / (right - left);
		dat[5] = 2.0f / (top - bottom);
		dat[10] = -2.0f / (zFar - zNear);
		dat[12] = -((right + left) / (right - left));
		dat[13] = -((top + bottom) / (top - bottom));
		dat[14] = -((zFar + zNear) / (zFar - zNear));
		dat[15] = 1.0f;

		return new mat4(dat, true);
	};

	static mat4 *Translate(mat4 m, vec3 pos) {
		mat4 *t_mat = CreateTranslationMatrix(pos),
		     *r = new mat4(m * *t_mat);
		delete t_mat;
		return r;
	}

	static mat4 *Rotate(mat4 m, float theta, vec3 axis) {
		vec3 a = vec3::Normalize(axis);
		float Cos = cos(theta), Sin = sin(theta), iCos = 1.0f - Cos;
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


			0.0f, 
			0.0f,
			0.0f, 
			1.0f
		};


		mat4* r_mat = new mat4(rMat);
		mat4 *r = new mat4(m * * r_mat);
		delete r_mat;
		return r;
	}

	static mat4 *LookAt(vec3 right, vec3 up, vec3 dir) {
		vec3 f = vec3::Normalize(up - right);
		vec3 s = vec3::Normalize(vec3::CrossProd(f, dir));
		vec3 u = vec3::CrossProd(s,f);

		float dat[16] = {
			s.x, u.x, -f.x, 0.0f,
			s.y, u.y, -f.y, 0.0f,
			s.z, u.z, -f.z, 0.0f,

			-vec3::DotProd(s, right),
			-vec3::DotProd(u, right),
			 vec3::DotProd(f, right),
			1.0f
		};

		return new mat4(dat);
	}

	~mat4() {
		this->attemptFree();
	}
};