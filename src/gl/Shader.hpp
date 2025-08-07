#pragma once
#include <iostream>
#include <fstream>
#include <glad.h>
#include "../msutil.hpp"
#include "../vec.hpp"
#include "../mat.hpp"

class Shader {
public:
    u32 PGRM, vert, frag;
	Shader(const char *vertex_data, const char *fragment_data);
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

	static Shader LoadShaderFromResource(std::string asset_path, std::string map_loc, std::string vert_id, std::string frag_id);
	static Shader LoadShaderFromFile(std::string vert_path, std::string frag_path);
private:
	enum class ShaderType {
		program,
		frag,
		vert
	};
	void __error_check(u32 shader, ShaderType type);
};