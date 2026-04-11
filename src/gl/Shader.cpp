#include "Shader.hpp"
#include <cstring>

void Shader::__error_check(u32 shader, ShaderType type) {
	i32 s = 0;
	char infLog[1024];

    std::cout << "Checking shader errors..." << std::endl;

	if (type != ShaderType::program) {
		glGetShaderiv(shader, GL_COMPILE_STATUS, &s);

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

    std::cout << "CREATED PROGRAM: " << this->PGRM << std::endl; 
	//add the shaders
	glAttachShader(this->PGRM, this->vert);
	glAttachShader(this->PGRM, this->frag);
	//link the program to the GPU
	glLinkProgram(this->PGRM);

	//check errors
	__error_check(this->PGRM, ShaderType::program);
    

    std::cout << "Shader gen done" << std::endl;
	//delete shaders since were done
	//glDeleteShader(vertex);
	//glDeleteShader(fragment);
}

Shader::Shader(const char *vertex_data, const char *fragment_data, const char *tcs_data, const char *tes_data) {
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

    //tcs
    this->tcs = glCreateShader(GL_TESS_CONTROL_SHADER);

    glShaderSource(this->tcs, 1, &tcs_data, NULL);
    glCompileShader(this->tcs);

    __error_check(this->tcs, ShaderType::tcs);

    //tes
    this->tes = glCreateShader(GL_TESS_EVALUATION_SHADER);

    glShaderSource(this->tes, 1, &tes_data, NULL);
    glCompileShader(this->tes);

    __error_check(this->tes, ShaderType::tes);

	//create the program
	this->PGRM = glCreateProgram();

    std::cout << "CREATED PROGRAM: " << this->PGRM << std::endl; 
	//add the shaders
	glAttachShader(this->PGRM, this->vert);
	glAttachShader(this->PGRM, this->frag);
    glAttachShader(this->PGRM, this->tcs);
	glAttachShader(this->PGRM, this->tes);
	//link the program to the GPU
	glLinkProgram(this->PGRM);

	//check errors
	__error_check(this->PGRM, ShaderType::program);
    

    std::cout << "Shader gen done" << std::endl;
	//delete shaders since were done
	//glDeleteShader(vertex);
	//glDeleteShader(fragment);
}

i32 Shader::SetVec2(std::string label, vec2 *v) {
    if (this->PGRM == NULL)
        return 1;

    GLint loc = glGetUniformLocation(this->PGRM, label.c_str());
    //if (loc == NULL)
    //    return 2;

    glUniform2f(loc, v->x, v->y);

    if (this->uni_save) {
        const size_t dat_access_max = NUQUERY_DAT_BYTES / sizeof(f32);

        if (dat_access_max < 2) {
            std::cout << "Cannot save vec2! DAT is too small." << std::endl;
            return 0;
        }

        _uquery_val uqv = {
            .loc = loc,
            .sty = _univ_super_ty::_F,
            .ty = _univ_ty::_vec2
        };

        f32 *i_dat = (f32*) uqv.DAT;

        i_dat[0] = v->x;
        i_dat[1] = v->y;
            
        this->Qry_stack_push(uqv);
    }

    return 0;
}

i32 Shader::SetVec3(std::string label, vec3 *v) {
    if (this->PGRM == NULL)
        return 1;

    GLint loc = glGetUniformLocation(this->PGRM, label.c_str());
    //if (loc == NULL)
    //    return 2;

    glUniform3f(loc, v->x, v->y, v->z);

    if (this->uni_save) {
        const size_t dat_access_max = NUQUERY_DAT_BYTES / sizeof(f32);

        if (dat_access_max < 3) {
            std::cout << "Cannot save vec3! DAT is too small." << std::endl;
            return 0;
        }

        _uquery_val uqv = {
            .loc = loc,
            .sty = _univ_super_ty::_F,
            .ty = _univ_ty::_vec3
        };

        f32 *i_dat = (f32*) uqv.DAT;

        i_dat[0] = v->x;
        i_dat[1] = v->y;
        i_dat[2] = v->z;
            
        this->Qry_stack_push(uqv);
    }

    return 0;
}

i32 Shader::SetVec4(std::string label, vec4 *v) {
    if (this->PGRM == NULL)
        return 1;

    GLint loc = glGetUniformLocation(this->PGRM, label.c_str());
    //if (loc == NULL)
    //    return 2;

    glUniform4f(loc, v->x, v->y, v->z, v->w);

    if (this->uni_save) {
        const size_t dat_access_max = NUQUERY_DAT_BYTES / sizeof(f32);

        if (dat_access_max < 4) {
            std::cout << "Cannot save vec4! DAT is too small." << std::endl;
            return 0;
        }

        _uquery_val uqv = {
            .loc = loc,
            .sty = _univ_super_ty::_F,
            .ty = _univ_ty::_vec4
        };

        f32 *i_dat = (f32*) uqv.DAT;

        i_dat[0] = v->x;
        i_dat[1] = v->y;
        i_dat[2] = v->z;
        i_dat[3] = v->w;
            
        this->Qry_stack_push(uqv);
    }

    return 0;
}

i32 Shader::SetMat4(std::string label, mat4 *m) {
    if (this->PGRM == 0)
        return 1;

    GLint loc = glGetUniformLocation(this->PGRM, label.c_str());
    //if (loc == 0)
        //return 2;

    glUniformMatrix4fv(loc, 1, GL_FALSE, m->glPtr());

    if (this->uni_save) {
        const size_t dat_access_max = NUQUERY_DAT_BYTES / sizeof(f32);

        if (dat_access_max < 16) {
            std::cout << "Cannot save mat4! DAT is too small." << std::endl;
            return 0;
        }

        _uquery_val uqv = {
            .loc = loc,
            .sty = _univ_super_ty::_F,
            .ty = _univ_ty::_mat4
        };

        f32 *f_dat = (f32*) uqv.DAT;

        in_memcpy(f_dat, m->glPtr(), sizeof(f32) * 16);
            
        this->Qry_stack_push(uqv);
    }

    return 0;
}

i32 Shader::SetiVec2(std::string label, vec2 *v) {
    if (this->PGRM == 0)
        return 1;

    GLint loc = glGetUniformLocation(this->PGRM, label.c_str());
    //if (loc == NULL)
    //    return 2;

    glUniform2i(loc, (i32) v->x, (i32) v->y);

    if (this->uni_save) {
        const size_t dat_access_max = NUQUERY_DAT_BYTES / sizeof(i32);

        if (dat_access_max < 2) {
            std::cout << "Cannot save vec2! DAT is too small." << std::endl;
            return 0;
        }

        _uquery_val uqv = {
            .loc = loc,
            .sty = _univ_super_ty::_I,
            .ty = _univ_ty::_vec2
        };

        i32 *i_dat = (i32*) uqv.DAT;

        i_dat[0] = v->x;
        i_dat[1] = v->y;
            
        this->Qry_stack_push(uqv);
    }

    return 0;
}

i32 Shader::SetiVec3(std::string label, vec3 *v) {
    if (this->PGRM == NULL)
        return 1;

    GLint loc = glGetUniformLocation(this->PGRM, label.c_str());
    //if (loc == NULL)
    //    return 2;

    glUniform3i(loc, (i32) v->x, (i32) v->y, (i32) v->z);

    if (this->uni_save) {
        const size_t dat_access_max = NUQUERY_DAT_BYTES / sizeof(i32);

        if (dat_access_max < 3) {
            std::cout << "Cannot save vec3! DAT is too small." << std::endl;
            return 0;
        }

        _uquery_val uqv = {
            .loc = loc,
            .sty = _univ_super_ty::_I,
            .ty = _univ_ty::_vec3
        };

        i32 *i_dat = (i32*) uqv.DAT;

        i_dat[0] = v->x;
        i_dat[1] = v->y;
        i_dat[2] = v->z;
            
        this->Qry_stack_push(uqv);
    }

    return 0;
}

i32 Shader::SetiVec4(std::string label, vec4 *v) {
    if (this->PGRM == NULL)
        return 1;

    GLint loc = glGetUniformLocation(this->PGRM, label.c_str());
    //if (loc == NULL)
    //    return 2;

    glUniform4i(loc, (i32) v->x, (i32) v->y, (i32) v->z, (i32) v->w);

    if (this->uni_save) {
        const size_t dat_access_max = NUQUERY_DAT_BYTES / sizeof(i32);

        if (dat_access_max < 4) {
            std::cout << "Cannot save vec4! DAT is too small." << std::endl;
            return 0;
        }

        _uquery_val uqv = {
            .loc = loc,
            .sty = _univ_super_ty::_I,
            .ty = _univ_ty::_vec4
        };

        i32 *i_dat = (i32*) uqv.DAT;

        i_dat[0] = v->x;
        i_dat[1] = v->y;
        i_dat[2] = v->z;
        i_dat[3] = v->w;
            
        this->Qry_stack_push(uqv);
    }

    return 0;
}

i32 Shader::SetBool(std::string label, bool b) {
    return this->SetInt(label, (i32)b);
}

i32 Shader::SetInt(std::string label, i32 val) {
    if (this->PGRM == NULL)
        return 1;

    GLint loc = glGetUniformLocation(this->PGRM, label.c_str());
    //if (loc == NULL)
    //    return 2;

    glUniform1i(loc, val);

    if (this->uni_save) {
        const size_t dat_access_max = NUQUERY_DAT_BYTES / sizeof(i32);

        if (dat_access_max < 1) {
            std::cout << "Cannot save int! DAT is too small." << std::endl;
            return 0;
        }

        _uquery_val uqv = {
            .loc = loc,
            .sty = _univ_super_ty::_I,
            .ty = _univ_ty::_num
        };

        i32 *i_dat = (i32*) uqv.DAT;
        i_dat[0] = val;
            
        this->Qry_stack_push(uqv);
    }

    return 0;
}

i32 Shader::SetFloat(std::string label, f32 v) {
    if (this->PGRM == NULL)
        return 1;

    GLint loc = glGetUniformLocation(this->PGRM, label.c_str());
    //if (loc == NULL)
    //    return 2;

    glUniform1f(loc, v);

    if (this->uni_save) {
        const size_t dat_access_max = NUQUERY_DAT_BYTES / sizeof(f32);

        if (dat_access_max < 1) {
            std::cout << "Cannot save float! DAT is too small." << std::endl;
            return 0;
        }

        _uquery_val uqv = {
            .loc = loc,
            .sty = _univ_super_ty::_F,
            .ty = _univ_ty::_num
        };

        f32 *f_dat = (f32*) uqv.DAT;
        f_dat[0] = v;
            
        this->Qry_stack_push(uqv);
    }

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

    std::cout << "loading shaders from: " << vert_path << " and " << frag_path << std::endl;

    //file v_file = FileWrite::readFromBin(Path::GetOSPath(vert_path)),
    //     f_file = FileWrite::readFromBin(Path::GetOSPath(frag_path));

    file v_file = FileWrite::readFromBin(vert_path),
         f_file = FileWrite::readFromBin(frag_path);

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

    std::cout << "E" << std::endl;

    Shader s = Shader(
        vertCode,
        fragCode
    );

    _safe_free_a(vertCode);
    _safe_free_a(fragCode);

    return s;
}

Shader Shader::LoadShaderFromFile(std::string vert_path, std::string frag_path, std::string tcs_path, std::string tes_path) {
    if (vert_path.length() == 0 || frag_path.length() == 0 || tcs_path.length() == 0 || tes_path.length() == 0) {
        std::cout << "Invalid shader path!" << std::endl;
        return {};
    }

    std::cout << "loading shaders from: " << vert_path << " and " << frag_path << " and these also: " << tcs_path << " " << tes_path << std::endl;

    //file v_file = FileWrite::readFromBin(Path::GetOSPath(vert_path)),
    //     f_file = FileWrite::readFromBin(Path::GetOSPath(frag_path));

    file v_file = FileWrite::readFromBin(vert_path),
         f_file = FileWrite::readFromBin(frag_path),
         c_file = FileWrite::readFromBin(tcs_path),
         e_file = FileWrite::readFromBin(tes_path);

    if (v_file.len == 0 || f_file.len == 0 || c_file.len == 0 || e_file.len == 0 || !v_file.dat || !f_file.dat || !c_file.dat || !e_file.dat) {
        std::cout << "Failed to read shader file! Likely invalid path!" << std::endl;
        if (v_file.dat) _safe_free_a(v_file.dat);
        if (f_file.dat) _safe_free_a(f_file.dat);
        if (c_file.dat) _safe_free_a(c_file.dat);
        if (e_file.dat) _safe_free_a(e_file.dat);
        return {};
    }

    const char *vertCode = CovertBytesToString(v_file.dat, v_file.len, true),
               *fragCode = CovertBytesToString(f_file.dat, f_file.len, true),
               *tcsCode = CovertBytesToString(c_file.dat, c_file.len, true),
               *tesCode = CovertBytesToString(e_file.dat, e_file.len, true);

    if (!vertCode || !fragCode || !tcsCode || !tesCode) {
        std::cout << "Failed to convert vertex or fragment data!" << std::endl;
        if (vertCode) _safe_free_a(vertCode);
        if (fragCode) _safe_free_a(fragCode);
        if (tcsCode) _safe_free_a(tcsCode);
        if (tesCode) _safe_free_a(tesCode);
        return {};
    }

    std::cout << "E" << std::endl;

    Shader s = Shader(
        vertCode,
        fragCode,
        tcsCode,
        tesCode
    );

    _safe_free_a(vertCode);
    _safe_free_a(fragCode);
    _safe_free_a(tcsCode);
    _safe_free_a(tesCode);

    return s;
}

void Shader::alloc_qstack() {
    if (this->Qry_Stack) return;
    this->Qry_Stack = new _uquery_val[_qstack_sz_delta];
    ZeroMem(this->Qry_Stack, _qstack_sz_delta);
    
    this->qstack_sz = _qstack_sz_delta;
    this->qs_cur = this->Qry_Stack;
    this->Qs_end = this->Qry_Stack + _qstack_sz_delta;
}

void Shader::free_qstack() {
    if (this->Qry_Stack)
        delete[] this->Qry_Stack;

    this->Qry_Stack = nullptr;
    this->qstack_sz = 0;
}

void Shader::inc_qstack_sz() {
    if (!this->Qry_Stack)
        this->alloc_qstack();

    const size_t newSz = this->qstack_sz + _qstack_sz_delta;

    auto *qs_new = new _uquery_val[newSz];
    ZeroMem(this->Qry_Stack, newSz);
    in_memcpy(qs_new, this->Qry_Stack, this->qstack_sz * sizeof(_uquery_val));
    
    const size_t curOff = (size_t) ((uintptr_t)this->qs_cur - (uintptr_t)this->Qry_Stack);

    this->Qs_end = qs_new + newSz;
    this->qs_cur = qs_new + curOff;

    delete[] this->Qry_Stack;

    this->Qry_Stack = qs_new;
    this->qstack_sz = newSz;
}


void Shader::clr_qstack() {
    if (!this->Qry_Stack)
        return;

    ZeroMem(this->Qry_Stack, this->qstack_sz);
    this->qs_cur = this->Qry_Stack;
}

Shader::_uquery_val *Shader::Qry_stack_pop() {
    if (!this->Qry_Stack)
        this->alloc_qstack();

    _uquery_val *val = this->qs_cur;

    if (this->qs_cur > this->Qry_Stack)
        this->qs_cur--;

    return val;
}

void Shader::Qry_stack_push(Shader::_uquery_val uqv) {
    if (!this->Qry_Stack)
        this->alloc_qstack();

    std::cout << "pushing uniform: " << uqv.loc << std::endl;

    *this->qs_cur = uqv;

    if (++this->qs_cur >= this->Qs_end)
        this->inc_qstack_sz();
}

void Shader::Delete() {
    this->free_qstack();
}

void Shader::usaveRestore() {
    if (!this->Qry_Stack || this->qstack_sz == 0 || !this->uni_save) {
        return;
    }

    _uquery_val *uqv = this->Qry_Stack;

    const size_t idat_access_max = NUQUERY_DAT_BYTES / sizeof(i32);
    const size_t fdat_access_max = NUQUERY_DAT_BYTES / sizeof(f32);

    f32 *f_dat = (f32*) &uqv->DAT;
    i32 *i_dat = (i32*) &uqv->DAT;

    while (uqv < this->qs_cur) {
        std::cout << "restoring uniform: " << (i32) uqv->sty << " " << (i32) uqv->ty << std::endl;

        switch (uqv->sty) {
        case _univ_super_ty::_F:
            switch (uqv->ty) {
            case _univ_ty::_num:
                if (fdat_access_max < 1) {
                    std::cout << "Cannot restore float uniform! DAT is too small!" << std::endl;
                    break;
                }
                glUniform1f(uqv->loc, f_dat[0]);
                break;
            case _univ_ty::_vec2:
                if (fdat_access_max < 2) {
                    std::cout << "Cannot restore vec2 uniform! DAT is too small!" << std::endl;
                    break;
                }
                glUniform2f(uqv->loc, f_dat[0], f_dat[1]);
                break;
            case _univ_ty::_vec3:
                if (fdat_access_max < 3) {
                    std::cout << "Cannot restore vec3 uniform! DAT is too small!" << std::endl;
                    break;
                }
                glUniform3f(uqv->loc, f_dat[0], f_dat[1], f_dat[2]);
                break;
            case _univ_ty::_vec4:
                if (fdat_access_max < 4) {
                    std::cout << "Cannot restore vec4 uniform! DAT is too small!" << std::endl;
                    break;
                }
                glUniform4f(uqv->loc, f_dat[0], f_dat[1], f_dat[2], f_dat[3]);
                break;
            case _univ_ty::_mat4:
                if (fdat_access_max < 16) {
                    std::cout << "Cannot restore mat4 uniform! DAT is too small!" << std::endl;
                    break;
                }
                glUniformMatrix4fv(uqv->loc, 1, false, f_dat);
                break;
            default:
                std::cout << "Cannot restore unsupported uniform type: " << (i32) uqv->ty << std::endl;
                break;
            }
            break;
        case _univ_super_ty::_I:
            switch (uqv->ty) {
            case _univ_ty::_num:
                if (idat_access_max < 1) {
                    std::cout << "Cannot restore int uniform! DAT is too small!" << std::endl;
                    break;
                }
                glUniform1i(uqv->loc, i_dat[0]);
                break;
            case _univ_ty::_vec2:
                if (idat_access_max < 2) {
                    std::cout << "Cannot restore ivec2 uniform! DAT is too small!" << std::endl;
                    break;
                }
                glUniform2i(uqv->loc, i_dat[0], i_dat[1]);
                break;
            case _univ_ty::_vec3:
                if (idat_access_max < 3) {
                    std::cout << "Cannot restore ivec3 uniform! DAT is too small!" << std::endl;
                    break;
                }
                glUniform3i(uqv->loc, i_dat[0], i_dat[1], i_dat[2]);
                break;
            case _univ_ty::_vec4:
                if (idat_access_max < 4) {
                    std::cout << "Cannot restore ivec4 uniform! DAT is too small!" << std::endl;
                    break;
                }
                glUniform4i(uqv->loc, i_dat[0], i_dat[1], i_dat[2], i_dat[3]);
                break;
            case _univ_ty::_bool:
                if (idat_access_max < 1) {
                    std::cout << "Cannot restore bool uniform! DAT is too small!" << std::endl;
                    break;
                }
                glUniform1i(uqv->loc, i_dat[0]);
                break;
            default:
                std::cout << "Cannot restore unsupported integer uniform type: " << (i32) uqv->ty << std::endl;
                break;
            }
            break;
        default:
            std::cout << "Invalid base uniform type: " << (i32)uqv->sty << std::endl;
            break;
        }
        uqv++;
    }
}

void Shader::EnablePersistantUniforms() {
    this->uni_save = true;
}

void Shader::DisablePersistantUniforms() {
    this->uni_save = false;
}