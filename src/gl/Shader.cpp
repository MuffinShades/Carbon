#include "Shader.hpp"
#include <cstring>

void Shader::__error_check(u32 shader, ShaderType type) {
	i32 s = 0;
	char infLog[1024];

	if (type != ShaderType::program) {
		glGetShaderiv(shader, GL_LINK_STATUS, &s);

		if (!s) {
			glGetShaderInfoLog(shader, 1024, NULL, infLog);
			std::cout << "Shader Error: " << (u32) type << "\n" << infLog << std::endl << std::endl;
		}
	} else {
		glGetProgramiv(shader, GL_LINK_STATUS, &s);

		if (!s) {
			glGetProgramInfoLog(shader, 1024, NULL, infLog);
			std::cout << "Program Error: " << (u32) type << "\n" << infLog << std::endl << std::endl;
		}
	}
}

Shader::Shader(const char *vertex_data, const char *fragment_data) {
	//vertex and fragment shaders

	//create vertex shader
	this->vert = glCreateShader(GL_VERTEX_SHADER);

	//add source code to shader
	glShaderSource(this->vert, 1, &vertex_data, NULL);
	glCompileShader(this->vert);

	//check for errors
	__error_check(this->vert, ShaderType::vert);

    //fragment

	this->frag = glCreateShader(GL_FRAGMENT_SHADER);

	//add source code to fragment shader
	glShaderSource(this->frag, 1, &fragment_data, NULL);
	glCompileShader(this->frag);
	//check for errors
	__error_check(this->frag, ShaderType::frag);

	//create the program
	this->PGRM = glCreateProgram();
	//add the shaders
	glAttachShader(this->PGRM, this->vert);
	glAttachShader(this->PGRM, this->frag);
	//link the program to the GPU
	glLinkProgram(this->PGRM);
	//check errors
	__error_check(this->PGRM, ShaderType::program);

	//delete shaders since were done
	//glDeleteShader(vertex);
	//glDeleteShader(fragment);
}

i32 Shader::SetVec2(std::string label, vec2 *v) {
    if (this->PGRM == NULL)
        return 1;

    GLint loc = glGetUniformLocation(this->PGRM, label.c_str());
    if (loc == NULL)
        return 2;

    glUniform2f(loc, v->x, v->y);

    return 0;
}

i32 Shader::SetVec3(std::string label, vec3 *v) {
    if (this->PGRM == NULL)
        return 1;

    GLint loc = glGetUniformLocation(this->PGRM, label.c_str());
    if (loc == NULL)
        return 2;

    glUniform3f(loc, v->x, v->y, v->z);

    return 0;
}

i32 Shader::SetVec4(std::string label, vec4 *v) {
    if (this->PGRM == NULL)
        return 1;

    GLint loc = glGetUniformLocation(this->PGRM, label.c_str());
    if (loc == NULL)
        return 2;

    glUniform4f(loc, v->x, v->y, v->z, v->w);

    return 0;
}

i32 Shader::SetMat4(std::string label, mat4 *m) {
    if (this->PGRM == 0)
        return 1;

    GLint loc = glGetUniformLocation(this->PGRM, label.c_str());
    //if (loc == 0)
        //return 2;

    glUniformMatrix4fv(loc, 1, GL_FALSE, m->glPtr());

    return 0;
}

i32 Shader::SetiVec2(std::string label, vec2 *v) {
    if (this->PGRM == 0)
        return 1;

    GLint loc = glGetUniformLocation(this->PGRM, label.c_str());
    if (loc == NULL)
        return 2;

    glUniform2i(loc, (i32) v->x, (i32) v->y);

    return 0;
}

i32 Shader::SetiVec3(std::string label, vec3 *v) {
    if (this->PGRM == NULL)
        return 1;

    GLint loc = glGetUniformLocation(this->PGRM, label.c_str());
    if (loc == NULL)
        return 2;

    glUniform3i(loc, (i32) v->x, (i32) v->y, (i32) v->z);

    return 0;
}

i32 Shader::SetiVec4(std::string label, vec4 *v) {
    if (this->PGRM == NULL)
        return 1;

    GLint loc = glGetUniformLocation(this->PGRM, label.c_str());
    if (loc == NULL)
        return 2;

    glUniform4i(loc, (i32) v->x, (i32) v->y, (i32) v->z, (i32) v->w);

    return 0;
}

i32 Shader::SetBool(std::string label, bool b) {
    return this->SetInt(label, (i32)b);
}

i32 Shader::SetInt(std::string label, i32 val) {
    if (this->PGRM == NULL)
        return 1;

    GLint loc = glGetUniformLocation(this->PGRM, label.c_str());
    if (loc == NULL)
        return 2;

    glUniform1i(loc, val);

    return 0;
}

i32 Shader::SetFloat(std::string label, f32 v) {
    if (this->PGRM == NULL)
        return 1;

    GLint loc = glGetUniformLocation(this->PGRM, label.c_str());
    if (loc == NULL)
        return 2;

    glUniform1f(loc, v);

    return 0;
}

void Shader::use() {
    glUseProgram(this->PGRM);
}

#include "../asset.hpp"
#include "../game/assetManager.hpp"

Shader Shader::LoadShaderFromResource(std::string asset_path, std::string map_loc, std::string vert_id, std::string frag_id) {
    if (asset_path.length() == 0 || map_loc.length() == 0 || vert_id.length() == 0 || frag_id.length() == 0) {
        std::cout << "shader error: invalid resource paths!" << std::endl;
        return {};
    }

     Asset *v = AssetManager::ReqAsset(vert_id, asset_path, map_loc),
           *f = AssetManager::ReqAsset(frag_id, asset_path, map_loc);


    if (!v || !f || v->sz <= 0 || f->sz <= 0) {
        std::cout << "Error failed to load default shaders!" << std::endl;
        return {};
    }

    const char* vertCode = CovertBytesToString(v->bytes, v->sz);
    const char* fragCode = CovertBytesToString(f->bytes, f->sz);

    //actually load the shaders
    Shader s = Shader(
        vertCode,
        fragCode
    );

    v->free();
    f->free();
    _safe_free_a(vertCode);
    _safe_free_a(fragCode);

    return s;
}

Shader Shader::LoadShaderFromFile(std::string vert_path, std::string frag_path) {
    if (vert_path.length() == 0 || frag_path.length() == 0) {
        std::cout << "Invalid shader path!" << std::endl;
        return {};
    }

    file v_file = FileWrite::readFromBin(Path::GetOSPath(vert_path)),
         f_file = FileWrite::readFromBin(Path::GetOSPath(frag_path));

    if (v_file.len == 0 || f_file.len == 0 || !v_file.dat || !f_file.dat) {
        std::cout << "Failed to read shader file! Likely invalid path!" << std::endl;
        if (v_file.dat) _safe_free_a(v_file.dat);
        if (f_file.dat) _safe_free_a(f_file.dat);
        return {};
    }

    const char *vertCode = CovertBytesToString(v_file.dat, v_file.len, true),
               *fragCode = CovertBytesToString(f_file.dat, f_file.len, true);

    if (!vertCode || !fragCode) {
        std::cout << "Failed to convert vertex or fragment data!" << std::endl;
        if (vertCode) _safe_free_a(vertCode);
        if (fragCode) _safe_free_a(fragCode);
        return {};
    }

    Shader s = Shader(
        vertCode,
        fragCode
    );

    _safe_free_a(vertCode);
    _safe_free_a(fragCode);

    return s;
}