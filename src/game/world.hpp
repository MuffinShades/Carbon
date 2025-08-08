#include <iostream>
#include "../gl/mesh.hpp"

constexpr u32 chunkSizeX = 16, chunkSizeY = 16, chunkSizeZ = 16;

struct Chunk {
    Mesh mesh;
    vec3 pos;
    mat4 modelMat;
};

class World {
private:
    u32 seed = 0;
public:
    //TODO: this thing
    World(u32 seed){
        this->seed = seed;
    };
    Chunk genChunk(vec3 pos);
    void render();
};