#include "world.hpp"
#include "../gl/noise.hpp"
#include "cube.hpp"

//constants needed for treating a 1d buffer as a 3d buffer
constexpr size_t chunkPageSize1d = 1, 
                 chunkPageSize2d = chunkPageSize1d * chunkSizeX, 
                 chunkPageSize3d = chunkPageSize2d * chunkSizeY;

#define chunkBlock_select_3(sb) ((sb).x * chunkPageSize1d + (sb).y * chunkPageSize2d + (sb).z * chunkPageSize3d)
#define chunkBlock_select_3s(x,y,z) ((x) * chunkPageSize1d + (y) * chunkPageSize2d + (z) * chunkPageSize3d)

void World::SetAtlas(TexAtlas *atlas) {
    if (!atlas) return;
    this->atlas = atlas;
}

static void gen_chunk_mesh(Chunk* c) {
    if (!c || !c->b_data) return;

    DynamicMesh dm;
    Vertex *face_buffer = new Vertex[36];

    //TODO: mesh generation
    //first simple generator
    u32 x,y,z;
    uvec3 p, ps;

    for (y = 1; y < chunkSizeY-1; y++) {
        p.y = y;
        for (z = 1; z < chunkSizeZ-1; z++) {
            p.z = z;
            for (x = 1; x < chunkSizeX-1; x++) {
                p.x = x;

                //top +y
                if (c->b_data[chunkBlock_select_3s(x, y-1, z)] & 0x3FFF) {
                    Cube::GenFace(face_buffer, 6, CubeFace::Top, {1.0f, 1.0f, 1.0f}, c->pos + p);
                    dm.addMeshData(face_buffer, 6, false);
                }

                //bottom -y
                if (c->b_data[chunkBlock_select_3s(x, y+1, z)] & 0x3FFF) {
                    Cube::GenFace(face_buffer, 6, CubeFace::Top, {1.0f, 1.0f, 1.0f}, c->pos + p);
                    dm.addMeshData(face_buffer, 6, false);
                }

                //north -x
                if (c->b_data[chunkBlock_select_3s(x-1, y, z)] & 0x3FFF) {
                    Cube::GenFace(face_buffer, 6, CubeFace::North, {1.0f, 1.0f, 1.0f}, c->pos + p);
                    dm.addMeshData(face_buffer, 6, false);
                }

                //south +x
                if (c->b_data[chunkBlock_select_3s(x+1, y, z)] & 0x3FFF) {
                    Cube::GenFace(face_buffer, 6, CubeFace::South, {1.0f, 1.0f, 1.0f}, c->pos + p);
                    dm.addMeshData(face_buffer, 6, false);
                }

                //east -z
                if (c->b_data[chunkBlock_select_3s(x, y, z-1)] & 0x3FFF) {
                    Cube::GenFace(face_buffer, 6, CubeFace::East, {1.0f, 1.0f, 1.0f}, c->pos + p);
                    dm.addMeshData(face_buffer, 6, false);
                }

                //west +z
                if (c->b_data[chunkBlock_select_3s(x, y, z+1)] & 0x3FFF) {
                    Cube::GenFace(face_buffer, 6, CubeFace::West, {1.0f, 1.0f, 1.0f}, c->pos + p);
                    dm.addMeshData(face_buffer, 6, false);
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
    u32 x, y, z;
    
    Perlin::NoiseSettings ns;

    ns.freq = 0.02f;

    Chunk c;

    for (z = 0; z < chunkSizeZ; ++z) {
        for (x = 0; x < chunkSizeX; ++x) {
            y = floor(p.advNoise2d(vec2(pos.x+x,pos.z+z), ns)) * chunkSizeY;

            for (;y >= 0; y--)
                set_chunk_block_data(&c, uvec3(x,y,z), 1); //set a grass block at the position
        }
    }

    gen_chunk_mesh(&c);

    c.pos = pos;
    c.modelMat = mat4(1.0f);

    return c;
}