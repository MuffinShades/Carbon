#include <iostream>
#include "../gl/mesh.hpp"
#include "../gl/atlas.hpp"

constexpr u32 chunkSizeX = 16, chunkSizeY = 16, chunkSizeZ = 16;
constexpr size_t nBlocksPerChunk = chunkSizeX * chunkSizeY * chunkSizeZ;

struct Block {
    u16 id = 0;
};

struct Chunk {
    /*
    
    Chunk Store Format:

    current:  stores a u64 in the form

    ???????? ???????? ???????? ???????? ???????? ???????? ??IIIIII IIIIIIII

    where I (14bits) is the id of the block
    and ? are bits reserved for later use

    future: u64 is a pointer to a Block struct, these pointers are obtained from a 
            dictionary / hash map so > 64bits of data can be used for block but also
            allow for simple compression by using a pointer to a unique block with a given
            state. For example, with lighting, there may be many different "grass" blocks
            in the dictionary with the only difference being the light level. This is far
            more efficient, memory-wise, than storing a unique block struct for each individual
            block in a chunk. The dictionary would be stored in the world and when generating
            a chunk would require a pointer to a world object
    */
    u64 *b_data = nullptr;
    Mesh mesh;
    vec3 pos;
    mat4 modelMat;
};

class World {
private:
    u32 seed = 0;
    TexAtlas *atlas = nullptr;
    Perlin p;
    
public:
    //TODO: this thing
    World(u32 seed){
        this->seed = seed;
        p = Perlin(this->seed);
    };
    void SetAtlas(TexAtlas *atlas);
    Chunk genChunk(vec3 pos);
    void render();
};