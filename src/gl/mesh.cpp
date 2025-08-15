#include "mesh.hpp"

Mesh::Mesh() {}

void Mesh::setMeshData(struct Vertex *v, size_t nVerts, bool free_obj) {
    if (!v || nVerts == 0) return;

    this->verts = new Vertex[nVerts];
    ZeroMem(this->verts, nVerts);

    in_memcpy(this->verts, v, sizeof(struct Vertex) * nVerts);

    this->nVerts = nVerts;

    if (free_obj) _safe_free_a(v);
}

const struct Vertex *Mesh::data() const {
    return this->verts;
}

const size_t Mesh::size() const {
    return this->nVerts;
}

void Mesh::free() {
    if (this->verts)
        _safe_free_a(this->verts);
    
    this->nVerts = 0;
}

void DynamicMesh::free() {
    MeshChunk *c = this->rootChunk, *toFree;

    while (c) {
        _safe_free_a(c->vData);
        toFree = c;
        c = c->next;
        _safe_free_b(toFree);
    }

    this->rootChunk = (this->lastChunk = nullptr);
}

DynamicMesh::MeshChunk *DynamicMesh::add_chunk() {
    MeshChunk *c = new MeshChunk;

    c->nAllocVerts = this->vertsPerChunk;
    c->vData = new Vertex[c->nAllocVerts];

    ZeroMem(c->vData, c->nAllocVerts);

    if (this->lastChunk) {
        c->pos = this->lastChunk->pos + this->lastChunk->nAllocVerts;
        c->prev = this->lastChunk;
        this->lastChunk->next = c;
        this->lastChunk = c;
    } else {
        c->pos = 0;

        this->rootChunk = (this->lastChunk = c);
    }

    return c;
}

DynamicMesh::DynamicMesh() {
    this->rootChunk = (this->lastChunk = this->add_chunk());
    this->set_cur_chunk(this->rootChunk);
}

void DynamicMesh::set_cur_chunk(DynamicMesh::MeshChunk *chunk, bool adjustPos) {
    if (!chunk) return;

    this->lastChunk = chunk;
    this->cur = chunk->vData;

    if (adjustPos) {
        this->pos = chunk->pos;
    }
}

void DynamicMesh::addMeshData(Vertex *v, size_t nVerts, bool free_obj) {
    if (!v || nVerts == 0) {
        if (free_obj && v)
            _safe_free_a(v);
        return;
    }

    std::cout << "Posf: " << this->chunkPos << " | " << nVerts << std::endl;

    //split between chunks needed (left copies)
    //TODO: add new chunks and copy data while we're above chunk boundary
    while (this->chunkPos + nVerts >= this->lastChunk->nAllocVerts) {
        //split between 2 chunks
        if (!this->lastChunk->next)
            this->add_chunk();

        size_t vtxLeft = this->lastChunk->nAllocVerts - this->chunkPos;

        in_memcpy(this->cur, v, sizeof(Vertex) * vtxLeft);

        this->lastChunk->nVerts = this->lastChunk->nAllocVerts;

        v += vtxLeft;
        nVerts -= vtxLeft;
        this->cur += vtxLeft;
        this->set_cur_chunk(this->lastChunk);
    }

    //no split easy peasy (right copy)
    in_memcpy(this->cur, v, sizeof(Vertex) * nVerts);
    this->lastChunk->nVerts += nVerts;
    this->pos_inc(nVerts);

    if (free_obj && v)
        _safe_free_a(v);
}

void DynamicMesh::mergeMesh(Mesh m, bool free_obj) {
    const Vertex *m_dat = m.data();
    const size_t nv = m.size();

    if (!m_dat || nv == 0)
        return;
}

void DynamicMesh::mergeMesh(DynamicMesh dym, bool free_obj) {
    if (dym.nVerts == 0 || !dym.rootChunk)
        return;

    //copy over other mesh as just more chunks
    const size_t nv = this->lastChunk->nVerts;

    if (nv > 0) {
        Vertex *clip = new Vertex[nv];
        in_memcpy(clip, this->lastChunk->vData, sizeof(Vertex) * nv);
        _safe_free_a(this->lastChunk->vData);
        this->lastChunk->vData = clip;
        this->lastChunk->nAllocVerts = nv;
    }

    this->lastChunk->next = dym.rootChunk;
    this->add_chunk();
    this->set_cur_chunk(this->lastChunk); //adjust the pointers
}

Mesh DynamicMesh::genBasicMesh() {
    this->nVerts = 0;
    this->nAllocVerts = 0;

    MeshChunk *c = this->rootChunk;

    while (c) {
        this->nVerts += c->nVerts;
        this->nAllocVerts += c->nAllocVerts;
        c = c->next;
    }

    std::cout << "Number of Mesh Verticies: " << this->nVerts << " | Alloc Verts: " << this->nAllocVerts << std::endl;

    //
    Vertex *v = new Vertex[this->nVerts];

    ZeroMem(v, this->nVerts);

    c = this->rootChunk;
    
    Vertex *vi = v, *fv;

    while (c) {
        in_memcpy(vi, c->vData, c->nVerts * sizeof(Vertex));
        vi += c->nVerts;
        fv = vi;
        c = c->next;
    }

    this->rootChunk = nullptr;

    Mesh m;

    m.verts = v;
    m.nVerts = this->nVerts;

    return m;
}

void DynamicMesh::pos_inc(size_t sz, bool changeChunkPos) {
    if (sz == 0) return;

    this->cur += sz;

    if (changeChunkPos) {
        this->chunkPos += sz;

        while (this->chunkPos >= this->lastChunk->nAllocVerts) {
            this->chunkPos -= this->lastChunk->nAllocVerts;
            this->lastChunk = this->lastChunk->next;

            if (!this->lastChunk) {
                this->add_chunk();
                this->set_cur_chunk(this->lastChunk, false); //update internal pointers
            } else
                this->set_cur_chunk(this->lastChunk, false); //update internal pointers

            this->cur += this->chunkPos;
        }

        this->lastChunk->nVerts = this->chunkPos+1;
    }

    this->pos += sz;
}