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

void World::genChunk(Chunk *c, vec3 pos) {
    if (!c) return;

    i32 x, y, z, h;
    
    Perlin::NoiseSettings ns;

    ns.freq = 0.005f;
    ns.freqScale = 3.0f;
    ns.amp = 2.5f;
    ns.ampScale = 0.4f;
    ns.nLayers = 4;

    ini_chunk(c);

    for (z = 0; z < chunkSizeZ; ++z) {
        for (x = 0; x < chunkSizeX; ++x) {
            y = h = abs(floorf(this->p.advNoise2d(vec2(pos.x+x,pos.z+z), ns) * chunkSizeY));

            for (;y >= 0; y--)
                if (y < h)
                    set_chunk_block_data(c, uvec3(x,y,z), (u32) BlockID::Dirt);
                else
                    set_chunk_block_data(c, uvec3(x,y,z), (u32) BlockID::Grass);
        }
    }

    gen_chunk_mesh(c, this);

    c->pos = pos;
    c->modelMat = mat4(1.0f);
}

void World::render(graphics *g) {
    if (!g) return;

    Chunk c;

    Shader *s = g->getCurrentShader();

    forrange(this->nChunks) {
        c = this->chunkBuffer[i];

        if (!c.b_data || !c.mesh.data())
            continue;

        s->SetMat4("model_mat", &c.modelMat);

        g->mush_render(&c.mesh);
    }
}

void World::genChunks() {
    while (this->genStack.size() > 0) {
        Chunk *tc = this->genStack.front();
        this->genStack.pop();
        auto process = [&](Chunk *chonk) {
            this->genChunk(chonk, chonk->pos);

            if (!chonk->b_data) {
                std::cout << "Failed to generate chunk at: <" << chonk->pos.x << ", "  << chonk->pos.y << ", "  << chonk->pos.z << ">" << std::endl;
            }
        };
        this->t_pool.Exe(process, tc);
    } 
}

void World::chunkBufIni() {
    if (this->chunkBuffer) {
        Chunk c;
        forrange(this->nChunks) {
            c = this->chunkBuffer[i];
            free_chunk(&c);
        }

        _safe_free_a(this->chunkBuffer);
    }

    this->nChunks = this->renderDistance.x * 2 * 
                    this->renderDistance.z * 2 * 
                    this->renderDistance.y;
    this->chunkBuffer = new Chunk[this->nChunks];

    ZeroMem(this->chunkBuffer, this->nChunks);

    bool stack_empty = this->genStack.empty();

    Chunk *c = nullptr;

    ivec3 cGlobeIdx = ivec3(-this->renderDistance.x, -this->renderDistance.y, -this->renderDistance.z);

    forrange(this->nChunks) {
        c = this->chunkBuffer + i;
        c->pos = vec3(chunkSizeX, chunkSizeY, chunkSizeZ) * cGlobeIdx;

        if (++cGlobeIdx.x >= this->renderDistance.x) {
            cGlobeIdx.x = -this->renderDistance.x;
            if (++cGlobeIdx.z >= this->renderDistance.z) {
                cGlobeIdx.z = -this->renderDistance.z;
                cGlobeIdx.y++;
            }
        }

        this->genStack.push(c);
    }
}