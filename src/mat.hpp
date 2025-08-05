#pragma once
#include <iostream>
#include "vec.hpp"
#include "msutil.hpp"
#include <cmath>

#define MAT4_POINT(x,y) (x+(y << 2))
#define MAT4_MEMALLOC 16


class mat4 {
public:
	f32 m[16] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	void attemptFree();

	//initializers
	mat4();
	mat4(f32 *dat, bool free);
	mat4(f32 dat[MAT4_MEMALLOC]);

	/*
	Constructor for Identity matrix

	1 0 0 0
	0 1 0 0
	0 0 1 0
	0 0 0 1
	
	
	*/
	mat4(f32 value);

	const f32* glPtr();
	f32 *getObjPtr() {
		f32 *rp = new f32[16];
		in_memcpy(rp, this->m, 16 * sizeof(float));
		return rp;
	}

	//temp class for [][] see https://stackoverflow.com/questions/27718586/overloading-double-subscript-operator-in-c
	class _Mat4Row {
	private:
		f32* dat;
		size_t row;
	public:
		_Mat4Row(size_t row, f32 dat[16]) : row(row) { this->dat = dat; }
		f32 operator[](size_t colum);
	};

	//operator []
	_Mat4Row operator[](i32 row);

	mat4 operator*(mat4 m2);


	//utility functions
	static vec3 MultiplyMat4Vec(vec3 v, mat4 m);

    vec3 operator*(vec3 v);

	static mat4 CreatePersepctiveProjectionMatrix(float fov, float aspectRatio, float fNear, float fFar);
	static mat4 CreateRotationMatrixX(float theta, vec3 origin);
	static mat4 CreateRotationMatrixY(float theta, vec3 origin);
	static mat4 CreateRotationMatrixZ(float theta, vec3 origin);
	static mat4 CreateTranslationMatrix(vec3 pos);
	static mat4 CreateOrthoProjectionMatrix(float right, float left, float bottom, float top, float zNear, float zFar);
	static mat4 Translate(mat4 m, vec3 pos);
	static mat4 Rotate(mat4 m, float theta, vec3 axis);
	static mat4 LookAt(vec3 right, vec3 up, vec3 dir);

	~mat4() {
	}
};