#pragma once
#include <iostream>
#include "../bytestream.hpp"
#include "mesh.hpp"

class ObjParse {
public:
    static Mesh LoadMeshFromObjFile(std::string src);
};