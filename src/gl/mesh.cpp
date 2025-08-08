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

DynamicMesh::MeshChunk *DynamicMesh::add_chunk() {
    MeshChunk *c = new MeshChunk;

    c->nVerts = this->vertsPerChunk;
    c->vData = new Vertex[c->nVerts];

    ZeroMem(c->vData, c->nVerts);

    if (this->lastChunk) {
        c->pos = this->lastChunk->pos + this->lastChunk->nVerts;
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

    this->curChunk = chunk;
    this->cur = chunk->vData;

    if (adjustPos) {
        this->pos = chunk->pos;
    }
}

void DynamicMesh::addMeshData(Vertex *v, size_t nVerts, bool free_obj = false) {
    if (!v || nVerts == 0) {
        if (free_obj && v)
            _safe_free_a(v);
        return;
    }

    //split between chunks needed (left copies)
    //TODO: add new chunks and copy data while we're above chunk boundary
    while (this->chunkPos + nVerts >= this->curChunk->nVerts) {
        //split between 2 chunks
        if (!this->curChunk->next)
            this->add_chunk();

        //TODO: le copy
    }

    //no split easy peasy (right copy)
    in_memcpy(this->cur, v, sizeof(Vertex) * nVerts);
    this->pos_inc(nVerts);

    if (free_obj)
        _safe_free_a(v);
}

void DynamicMesh::mergeMesh(Mesh m, bool free_obj = false) {
    const Vertex *m_dat = m.data();
    const size_t nv = m.size();

    if (!m_dat || nv == 0)
        return;
}

void DynamicMesh::mergeMesh(DynamicMesh dym, bool free_obj = false) {
    if (dym.nVerts == 0 || !dym.rootChunk)
        return;

    //copy over other mesh as just more chunks
}

Mesh DynamicMesh::compress() {

}

void DynamicMesh::pos_inc(size_t sz) {
    if (sz == 0) return;

    this->cur += sz;
    this->chunkPos += sz;

    while (this->chunkPos > this->curChunk->nVerts) {
        this->chunkPos -= this->curChunk->nVerts;
        this->curChunk = this->curChunk->next;

        if (!this->curChunk) {
            this->add_chunk();
            this->set_cur_chunk(this->lastChunk, false); //update internal pointers
        } else
            this->set_cur_chunk(this->curChunk, false); //update internal pointers

        this->cur += this->chunkPos;
    }

    this->pos += sz;
}