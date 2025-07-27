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