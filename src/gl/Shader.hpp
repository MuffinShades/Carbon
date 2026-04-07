#pragma once
#include <iostream>
#include <fstream>
#include <glad.h>
#include "../msutil.hpp"
#include "../vec.hpp"
#include "../mat.hpp"
#include "sincl.hpp"

class Shader {
public:
    u32 PGRM = 0, vert, frag, tcs, tes;
	Shader(const char *vertex_data, const char *fragment_data);
	Shader(const char *vertex_data, const char *fragment_data, const char *tcs_data, const char *tes_data);
	Shader() {

	}
    i32 SetVec2(std::string label, vec2 *v);
    i32 SetVec3(std::string label, vec3 *v);
	i32 SetVec4(std::string label, vec4 *v);
    i32 SetMat4(std::string label, mat4 *m);
	i32 SetFloat(std::string label, f32 v);
	i32 SetInt(std::string label, i32 val);
	i32 SetBool(std::string label, bool b);
	i32 SetiVec2(std::string label, vec2 *v);
	i32 SetiVec3(std::string label, vec3 *v);
	i32 SetiVec4(std::string label, vec4 *v);
	void use();

	void EnablePersistantUniforms();
	void DisablePersistantUniforms();

	bool good() {return this->PGRM != 0;}
	static Shader LoadShaderFromResource(std::string asset_path, std::string map_loc, std::string vert_id, std::string frag_id);
	static Shader LoadShaderFromFile(std::string vert_path, std::string frag_path);
	static Shader LoadShaderFromFile(std::string vert_path, std::string frag_path, std::string tcs_path, std::string tes_path);

	void Delete();
private:
	enum class ShaderType {
		program,
		frag,
		vert,
		tcs,
		tes
	};
	void __error_check(u32 shader, ShaderType type);

	enum class _univ_ty {
		_vec2,_vec3,_vec4,_mat4,_num,_bool
	};

	enum class _univ_super_ty {
		unknown,
		_I, //integer
		_F //floating point
	};

	struct _uquery_val {
		i32 loc = -1;
		_univ_super_ty sty = _univ_super_ty::unknown;
		_univ_ty ty;
		byte DAT[64] = {0};
	};

	//todo: implement this for persistant uniforms
	_uquery_val *Qry_Stack = nullptr, *Qs_end = nullptr, * qs_cur = nullptr;
	size_t qstack_sz = 0;

	const size_t _qstack_sz_delta = 0xff; //how much query stack is changed by

	void alloc_qstack();
	void free_qstack();
	void inc_qstack_sz();
	void dec_qstack_sz();

	_uquery_val *Qry_stack_pop();
	void Qry_stack_push(_uquery_val uqv);
};