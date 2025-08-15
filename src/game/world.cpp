#include "world.hpp"
#include "../gl/noise.hpp"
#include "cube.hpp"

//constants needed for treating a 1d buffer as a 3d buffer
constexpr size_t chunkPageSize1d = 1, 
                 chunkPageSize2d = chunkPageSize1d * chunkSizeX, 
                 chunkPageSize3d = chunkPageSize2d * chunkSizeZ;

#define chunkBlock_select_3(sb) (((sb).x * chunkPageSize1d) + ((sb).z * chunkPageSize2d) + ((sb).y * chunkPageSize3d))
#define chunkBlock_select_3s(x,y,z) (((x) * chunkPageSize1d) + ((z) * chunkPageSize2d) + ((y) * chunkPageSize3d))

void World::SetAtlas(TexAtlas *atlas) {
    if (!atlas) return;
    this->atlas = atlas;
}

#include "mc_inf.hpp"

#define GenChunkFace(face) \
    texInf = bInf.tex[(u32)face]; \
    Cube::GenFace(face_buffer, 6, face, \
        TexAtlas::partToClip(atlas->getImageIndexPart(texInf.x, texInf.y)), \
    vec3(1.0f, 1.0f, 1.0f), c->pos + p); \
    dm.addMeshData(face_buffer, 6, false); 

static void gen_chunk_mesh(Chunk* c, World* w) {
    if (!c || !c->b_data) return;

    DynamicMesh dm;
    Vertex *face_buffer = new Vertex[36];

    //TODO: mesh generation
    //first simple generator
    u32 x,y,z;
    uvec3 p, ps;
    u32 nv = 0, blockId, data;

    TexAtlas *atlas = w->GetAtlas();

    BlockInf bInf;

    uvec2 texInf;

    if (!atlas) return;

    for (y = 0; y < chunkSizeY; y++) {
        p.y = y;
        for (z = 0; z < chunkSizeZ; z++) {
            p.z = z;
            for (x = 0; x < chunkSizeX; x++) {
                p.x = x;

                if (!(data = c->b_data[chunkBlock_select_3s(x, y, z)])) continue;

                blockId = data;

                if (blockId > (u32) BlockID::Unknown)
                    blockId = (u32) BlockID::Unknown;

                bInf = mc_blockData[blockId];

                //bottom -y
                if (y > 0 && c->b_data[chunkBlock_select_3s(x, y-1, z)] == 0) {
                    GenChunkFace(CubeFace::Bottom)
                }

                //top +y
                if (y < chunkSizeY - 1 && c->b_data[chunkBlock_select_3s(x, y+1, z)] == 0) {
                    GenChunkFace(CubeFace::Top)
                }

                //east -x
                if (x > 0 && c->b_data[chunkBlock_select_3s(x-1, y, z)] == 0) {
                    GenChunkFace(CubeFace::East)
                }

                //west +x
                if (x < chunkSizeX - 1 && c->b_data[chunkBlock_select_3s(x+1, y, z)] == 0) {
                    GenChunkFace(CubeFace::West)
                }

                //south -z
                if (z > 0 && c->b_data[chunkBlock_select_3s(x, y, z-1)] == 0) {
                    GenChunkFace(CubeFace::South)
                }

                //north +z
                if (z < chunkSizeZ - 1 && c->b_data[chunkBlock_select_3s(x, y, z+1)] == 0) {
                    GenChunkFace(CubeFace::North)
                }
            }
        }
    }

    c->mesh = dm.genBasicMesh();
    dm.free();

    _safe_free_a(face_buffer);
}

static void ini_chunk(Chunk* c) {
    if (!c) return;

    if (c->b_data) _safe_free_a(c->b_data);
    c->b_data = new u64[nBlocksPerChunk];
    ZeroMem(c->b_data, nBlocksPerChunk); //fill chunk with air :O
}

static void set_chunk_block_data(Chunk* c, uvec3 sb, u16 id) {
    const size_t p = chunkBlock_select_3(sb);
    if (!c || p >= nBlocksPerChunk) return;
    c->b_data[p] = id & 0x3FFF;
}

Chunk World::genChunk(vec3 pos) {
    i32 x, y, z, h;
    
    Perlin::NoiseSettings ns;

    ns.freq = 0.05f;
    ns.freqScale = 1.2f;
    ns.amp = 1.0f;
    ns.ampScale = 0.9f;
    ns.nLayers = 4;

    Chunk c;

    ini_chunk(&c);

    std::cout << "Generating Chunk..." << std::endl;

    for (z = 0; z < chunkSizeZ; ++z) {
        for (x = 0; x < chunkSizeX; ++x) {
            y = h = abs(floorf(this->p.advNoise2d(vec2(pos.x+x,pos.z+z), ns) * chunkSizeY));

            for (;y >= 0; y--)
                if (y < h)
                    set_chunk_block_data(&c, uvec3(x,y,z), (u32) BlockID::Dirt);
                else
                    set_chunk_block_data(&c, uvec3(x,y,z), (u32) BlockID::Grass);
        }
    }

    gen_chunk_mesh(&c, this);

    c.pos = pos;
    c.modelMat = mat4(1.0f);

    return c;
}