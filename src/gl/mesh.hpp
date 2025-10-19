#pragma once
#include <iostream>
#include "vertex.hpp"
#include "../vec.hpp"

struct MeshCard {
    i64 off = -1;
    void *buf = nullptr;
};

class Mesh {
private:
    Vertex *verts = nullptr;
    vec3 *normals = nullptr;
    size_t nVerts = 0, nFaces = 0;
    MeshCard card;
    bool dataOwn = true;

    void computeTriangleBasedNormals();
public:
    Mesh();
    const Vertex *data() const;
    const size_t size() const;
    const vec3 *getStoredTriangleBasedNormals();
    const size_t getNTriangleFaces();
    void setMeshData(Vertex *v, size_t nVerts, bool free_obj = false);
    void free();
    class StockMeshes {
    public:
        static Mesh GenerateCube();
    };

    friend class DynamicMesh;
    friend class CombinedMesh;
};

class DynamicMesh {
protected:
    const size_t vertsPerChunk = 0x4fff;

    struct MeshChunk {
        MeshChunk* next = nullptr, *prev = nullptr;
        Vertex* vData = nullptr;
        size_t nVerts = 0, nAllocVerts = 0, pos = 0;
        bool locked = false; //if locked then the buffer with be treated as a const Vertex*
    } *rootChunk = nullptr,
      *lastChunk = nullptr;
    Vertex *cur = nullptr;

    size_t nVerts = 0, nAllocVerts = 0, pos = 0, chunkPos = 0;

    MeshChunk *add_chunk();
    MeshChunk *add_chunk(size_t sz);
    void pos_inc(size_t sz, bool changeChunkPos = true);
    void set_cur_chunk(MeshChunk *chunk, bool adjustPos = true);

    MeshCard card;
public:
    DynamicMesh();
    const Vertex *data();
    const size_t size() const;
    void addMeshData(Vertex *v, size_t nVerts, bool free_obj = false);
    void mergeMesh(Mesh m, bool free_obj = false);
    void mergeMesh(DynamicMesh dym, bool free_obj = false);
    Mesh genBasicMesh();
    void free();

    friend class CombinedMesh;
};

/*

Overall goal: be able to combine a ton of meshes into 1 buffer without using memcpy
meshes with inherit

*/

class CombinedMesh : public DynamicMesh {
private:
    using DynamicMesh::genBasicMesh;
    using DynamicMesh::mergeMesh;

    struct FreeSpace {
        size_t nFreeVerts = 0;
        size_t chunkOffset = 0;
        DynamicMesh::MeshChunk *targetChunk = nullptr;
        FreeSpace *prev = nullptr, *next = nullptr;
    } *spaceStack;

    void chunk_clip();
    FreeSpace *free_stack_insert_search(size_t sz);
    bool fs_insert(FreeSpace *space, FreeSpace *at);
public:
    MeshCard AddStaticMeshPart(Mesh *m);
    MeshCard AddDynamicMeshPart(DynamicMesh *dy_m);
    void RemoveMesh(Mesh *m);
    void RemoveMesh(DynamicMesh *dy_m);
};