#include <iostream>
#include "../gl/mesh.hpp"
#include "../gl/atlas.hpp"
#include "../gl/noise.hpp"
#include "../mat.hpp"
#include "../vec.hpp"
#include "../gl/graphics.hpp"

constexpr u32 chunkSizeX = 32, chunkSizeY = 32, chunkSizeZ = 32;
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

static void free_chunk(Chunk *c) {
    if (!c) return;
    c->mesh.free();
    _safe_free_a(c->b_data);
    ZeroMem(c, 1);
}

#include "../silk.hpp"

class World {
private:
    u32 seed = 0;
    TexAtlas *atlas = nullptr;
    Perlin p;
    ivec3 renderDistance = ivec3(8, 1, 8);
    Chunk *chunkBuffer = nullptr;
    size_t nChunks = 0;
    std::queue<Chunk*> genStack;
    Silk::TPool t_pool;
     
public:
    //TODO: this thing
    World(u32 seed) : t_pool(4) {
        this->seed = seed;
        this->p = Perlin(this->seed);
    };
    TexAtlas *GetAtlas() const {
        return this->atlas;
    }
    void chunkBufIni();
    void SetAtlas(TexAtlas *atlas);
    void genChunk(Chunk *c, vec3 pos);
    void genChunks();
    void tick(vec3 pPos);
    void render(graphics *g);
};