#include "cube.hpp"

Vertex* Cube::GetFace(CubeFace f) {
    return (Vertex*) &cubeVerticies[(u32)f * 6];
}

Vertex* Cube::GenFace(CubeFace f, vec3 scale, vec3 off) {
    Vertex *fs = new Vertex[6];
    constexpr size_t fsz = 6 * sizeof(Vertex);
    ZeroMem(fs, 6);

    if (!fs ){
        std::cout << "error failed to generate face!" << std::endl;
        return nullptr;
    }

    Vertex v; Vertex *src = GetFace(f);
        
    //cant memcpy since we need to apply offsets and scaling
    forrange(6) {
        v = src[i];

        //scaling
        v.posf[0] *= scale.x;
        v.posf[1] *= scale.y;
        v.posf[2] *= scale.z;

        //offset
        v.posf[0] += off.x;
        v.posf[1] += off.y;
        v.posf[2] += off.z;

        fs[i] = v;
    }

    return fs;
}

bool Cube::GenFace(Vertex* fs, size_t bufSz, CubeFace f, vec3 scale, vec3 off) {
    if (bufSz < 6) {
        std::cout << "Cannot generate face to buffer smaller than 6 * sizeof(Vertex)!" << std::endl;
        return false;
    }

    constexpr size_t fsz = 6 * sizeof(Vertex);
    ZeroMem(fs, 6);

    if (!fs){
        std::cout << "error failed to generate face!" << std::endl;
        return false;
    }

    Vertex v; Vertex *src = GetFace(f);
        
    //cant memcpy since we need to apply offsets and scaling
    forrange(6) {
        v = src[i];

        //scaling
        v.posf[0] *= scale.x;
        v.posf[1] *= scale.y;
        v.posf[2] *= scale.z;

        //offset
        v.posf[0] += off.x;
        v.posf[1] += off.y;
        v.posf[2] += off.z;

        fs[i] = v;
    }

    return true;
}

Vertex* Cube::GenFace(CubeFace f, vec4 texClip, vec3 scale, vec3 off) {
    Vertex *fs = new Vertex[6];
    constexpr size_t fsz = 6 * sizeof(Vertex);
    ZeroMem(fs, 6);

    Vertex v; Vertex *src = GetFace(f);
        
    //cant memcpy since we need to apply offsets and scaling
    forrange(6) {
        v = src[i];

        //scaling
        v.posf[0] *= scale.x;
        v.posf[1] *= scale.y;
        v.posf[2] *= scale.z;

        //offset
        v.posf[0] += off.x;
        v.posf[1] += off.y;
        v.posf[2] += off.z;

        //different tex clip
        v.tex[0] = texClip.x + texClip.z * v.tex[0];
        v.tex[1] = texClip.y + texClip.w * v.tex[1];

        fs[i] = v;
    }

    return fs;
}

bool Cube::GenFace(Vertex* fs, size_t bufSz, CubeFace f, vec4 texClip, vec3 scale, vec3 off) {
    if (bufSz < 6) {
        std::cout << "Cannot generate face to buffer smaller than 6 * sizeof(Vertex)!" << std::endl;
        return false;
    }

    constexpr size_t fsz = 6 * sizeof(Vertex);
    ZeroMem(fs, 6);

    if (!fs){
        std::cout << "error failed to generate face!" << std::endl;
        return false;
    }

    Vertex v; Vertex *src = GetFace(f);
        
    //cant memcpy since we need to apply offsets and scaling
    forrange(6) {
        v = src[i];

        //scaling
        v.posf[0] *= scale.x;
        v.posf[1] *= scale.y;
        v.posf[2] *= scale.z;

        //offset
        v.posf[0] += off.x;
        v.posf[1] += off.y;
        v.posf[2] += off.z;

        //different tex clip
        v.tex[0] = texClip.x + texClip.z * v.tex[0];
        v.tex[1] = texClip.y + texClip.w * v.tex[1];

        fs[i] = v;
    }

    return true;
}