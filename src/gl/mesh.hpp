#pragma once
#include <iostream>
#include "graphics.hpp"

class Mesh {
private:
    Vertex *verts = nullptr;
    size_t nVerts = 0;
public:
    Mesh();
    const Vertex *data() const;
    const size_t size() const;
    void setMeshData(Vertex *v, size_t nVerts, bool free_obj = false);
    void free();
    class StockMeshes {
    public:
        static Mesh GenerateCube();
    };
};

class DynamicMesh {
private:
    const size_t vertsPerChunk = 0x4fff;

    struct MeshChunk {
        MeshChunk* next = nullptr, *prev = nullptr;
        Vertex* vData = nullptr;
        size_t nVerts, pos = 0;
    } *rootChunk = nullptr, 
      *curChunk = nullptr, 
      *lastChunk = nullptr;
    Vertex *cur = nullptr;
    size_t nVerts = 0, nAllocVerts = 0, pos = 0, chunkPos = 0;

    MeshChunk *add_chunk();
    void pos_inc(size_t sz);
    void set_cur_chunk(MeshChunk *chunk, bool adjustPos = true);
public:
    DynamicMesh();
    const Vertex *data() const;
    const size_t size() const;
    void addMeshData(Vertex *v, size_t nVerts, bool free_obj = false);
    void mergeMesh(Mesh m, bool free_obj = false);
    void mergeMesh(DynamicMesh dym, bool free_obj = false);
    Mesh compress();
    void free();
};