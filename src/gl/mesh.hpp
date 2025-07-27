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
