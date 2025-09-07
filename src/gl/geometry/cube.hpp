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
    static Vertex* GetFace(CubeFace f);
    static Vertex* GenFace(CubeFace f, vec3 scale = {1.0f, 1.0f, 1.0f}, vec3 off = {0.0f, 0.0f, 0.0f});
    static bool GenFace(Vertex* fs, size_t bufSz, CubeFace f, vec3 scale = {1.0f, 1.0f, 1.0f}, vec3 off = {0.0f, 0.0f, 0.0f});
    static Vertex* GenFace(CubeFace f, vec4 texClip, vec3 scale = {1.0f, 1.0f, 1.0f}, vec3 off = {0.0f, 0.0f, 0.0f});
    static bool GenFace(Vertex* fs, size_t bufSz, CubeFace f, vec4 texClip, vec3 scale = {1.0f, 1.0f, 1.0f}, vec3 off = {0.0f, 0.0f, 0.0f});
};