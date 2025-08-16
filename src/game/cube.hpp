#include "../gl/graphics.hpp"

constexpr struct Vertex cubeVerticies[] = {
    //NORTH
    0, 0, 1,  0, 0, 1,  0.0f, 1.0f,
    1, 1, 1,  0, 0, 1,  1.0f, 0.0f,
    1, 0, 1,  0, 0, 1,  1.0f, 1.0f,

    1, 1, 1,  0, 0, 1,  1.0f, 0.0f,
    0, 0, 1,  0, 0, 1,  0.0f, 1.0f,
    0, 1, 1,  0, 0, 1,  0.0f, 0.0f,

    //SOUTH
    0, 0, 0,  0, 0, -1,  1.0f, 1.0f,
    1, 0, 0,  0, 0, -1,  0.0f, 1.0f,
    1, 1, 0,  0, 0, -1,  0.0f, 0.0f,

    1, 1, 0,  0, 0, -1,  0.0f, 0.0f,
    0, 1, 0,  0, 0, -1,  1.0f, 0.0f,
    0, 0, 0,  0, 0, -1,  1.0f, 1.0f,

    //EAST
    0, 1, 1,  -1, 0, 0,  1.0f, 0.0f,
    0, 0, 0,  -1, 0, 0,  0.0f, 1.0f,
    0, 1, 0,  -1, 0, 0,  0.0f, 0.0f,

    0, 0, 0,  -1, 0, 0,  0.0f, 1.0f,
    0, 1, 1,  -1, 0, 0,  1.0f, 0.0f,
    0, 0, 1,  -1, 0, 0,  1.0f, 1.0f,

    //WEST
    1, 1, 1,  1, 0, 0,  1.0f, 0.0f,
    1, 1, 0,  1, 0, 0,  0.0f, 0.0f,
    1, 0, 0,  1, 0, 0,  0.0f, 1.0f,

    1, 0, 0,  1, 0, 0,  0.0f, 1.0f,
    1, 0, 1,  1, 0, 0,  1.0f, 1.0f,
    1, 1, 1,  1, 0, 0,  1.0f, 0.0f,

    //TOP
    0, 1, 0,  0, 1, 0,  0.0f, 1.0f,
    1, 1, 0,  0, 1, 0,  1.0f, 1.0f,
    1, 1, 1,  0, 1, 0,  1.0f, 0.0f,

    1, 1, 1,  0, 1, 0,  1.0f, 0.0f,
    0, 1, 1,  0, 1, 0,  0.0f, 0.0f,
    0, 1, 0,  0, 1, 0,  0.0f, 1.0f,

    //BOTTOM
    0, 0, 0,  0, -1, 0,  0.0f, 1.0f,
    1, 0, 1,  0, -1, 0,  1.0f, 0.0f,
    1, 0, 0,  0, -1, 0,  1.0f, 1.0f,

    1, 0, 1,  0, -1, 0,  1.0f, 0.0f,
    0, 0, 0,  0, -1, 0,  0.0f, 1.0f,
    0, 0, 1,  0, -1, 0,  0.0f, 0.0f
};

enum class CubeFace {
    North,
    South,
    East,
    West,
    Top,
    Bottom
};

class Cube {
public:
    static Vertex* GetFace(CubeFace f) {
        return (Vertex*) &cubeVerticies[(u32)f * 6];
    }

    static Vertex* GenFace(CubeFace f, vec3 scale = {1.0f, 1.0f, 1.0f}, vec3 off = {0.0f, 0.0f, 0.0f}) {
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

    static bool GenFace(Vertex* fs, size_t bufSz, CubeFace f, vec3 scale = {1.0f, 1.0f, 1.0f}, vec3 off = {0.0f, 0.0f, 0.0f}) {
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

    static Vertex* GenFace(CubeFace f, vec4 texClip, vec3 scale = {1.0f, 1.0f, 1.0f}, vec3 off = {0.0f, 0.0f, 0.0f}) {
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

    static bool GenFace(Vertex* fs, size_t bufSz, CubeFace f, vec4 texClip, vec3 scale = {1.0f, 1.0f, 1.0f}, vec3 off = {0.0f, 0.0f, 0.0f}) {
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
};