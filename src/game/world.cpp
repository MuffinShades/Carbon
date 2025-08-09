#include "world.hpp"
#include "../gl/noise.hpp"
#include "cube.hpp"

Chunk World::genChunk(vec3 pos) {
    u32 x, z;

    DynamicMesh dm;

    for (z = 0; z < chunkSizeZ; ++z) {
        for (x = 0; x < chunkSizeX; ++x) {
            dm.addMeshData(
                Cube::GenFace(
                    CubeFace::North, vec3(1.0f, 1.0f, 1.0f), 
                    pos + vec3(x, 0.0f, z)
                ), 6, 
                true
            );
        }
    }

    Chunk c;

    c.mesh = dm.genBasicMesh();
    c.pos = pos;
    c.modelMat = mat4(1.0f);

    dm.free();

    return c;
}