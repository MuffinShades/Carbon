#include "../gl/graphics.hpp"

constexpr struct Vertex cubeVerticies[] = {
    //NORTH
    0, 0, 1,  0.0f, 1.0f,
    1, 1, 1,  1.0f, 0.0f,
    1, 0, 1,  1.0f, 1.0f,

    1, 1, 1,  1.0f, 0.0f,
    0, 0, 1,  0.0f, 1.0f,
    0, 1, 1,  0.0f, 0.0f,

    //SOUTH
    0, 0, 0,  1.0f, 1.0f,
    1, 0, 0,  0.0f, 1.0f,
    1, 1, 0,  0.0f, 0.0f,

    1, 1, 0,  0.0f, 0.0f,
    0, 1, 0,  1.0f, 0.0f,
    0, 0, 0,  1.0f, 1.0f,

    //EAST
    0, 1, 1,  1.0f, 0.0f,
    0, 0, 0,  0.0f, 1.0f,
    0, 1, 0,  0.0f, 0.0f,

    0, 0, 0,  0.0f, 1.0f,
    0, 1, 1,  1.0f, 0.0f,
    0, 0, 1,  1.0f, 1.0f,

    //WEST
    1, 1, 1,  1.0f, 0.0f,
    1, 1, 0,  0.0f, 0.0f,
    1, 0, 0,  0.0f, 1.0f,

    1, 0, 0,  0.0f, 1.0f,
    1, 0, 1,  1.0f, 1.0f,
    1, 1, 1,  1.0f, 0.0f,

    //TOP
    0, 1, 0,  0.0f, 1.0f,
    1, 1, 0,  1.0f, 1.0f,
    1, 1, 1,  1.0f, 0.0f,

    1, 1, 1,  1.0f, 0.0f,
    0, 1, 1,  0.0f, 0.0f,
    0, 1, 0,  0.0f, 1.0f,

    //BOTTOM
    0, 0, 0,  0.0f, 1.0f,
    1, 0, 1,  1.0f, 0.0f,
    1, 0, 0,  1.0f, 1.0f,

    1, 0, 1,  1.0f, 0.0f,
    0, 0, 0,  0.0f, 1.0f,
    0, 0, 1,  0.0f, 0.0f
};

enum class CubeFace {
    North = 0,
    South = 6,
    East = 12,
    West = 18,
    Top = 24,
    Bottom = 30
};

class Cube {
public:
    static Vertex* GetFace(CubeFace f) {
        return ((Vertex*)cubeVerticies + ((u32)f * sizeof(Vertex)));
    }

    static Vertex* GenFace(CubeFace f, vec3 scale = {1.0f, 1.0f, 1.0f}, vec3 off = {0.0f, 0.0f, 0.0f}) {
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

            fs[i] = v;
        }

        return fs;
    }
};