#pragma once
#include "tokens.hpp"

struct BasicType {
    size_t sz;
    std::string name;
    std::vector<struct TypeAttr> attrs;
    bool u = false; //unsigned
    bool prim = false;
};

struct TypeAttr {
    std::string name;
    BasicType ty;
};

struct CustomType : public BasicType {
    Token origin;
};

const BasicType primitiveTypes[] = {
    //bool primitive type
    {
        .sz = 1,
        .name = "bool",
        .prim = true
    },

    //char primitive type
    {
        .sz = 1,
        .name = "char",
        .prim = true
    },

    //byte primitive type
    //basically just unsigned char
    {
        .sz = 1,
        .name = "byte",
        .u = true,
        .prim = true
    },

    //short primitive type
    {
        .sz = 2,
        .name = "i16",
        .prim = true
    },

    //int primitive type
    {
        .sz = 4,
        .name = "i32",
        .prim = true
    },

    //long primitive type
    {
        .sz = 8,
        .name = "i64",
        .prim = true
    },

    //light primitive type (float but as a short)
    {
        .sz = 2,
        .name = "f16",
        .prim = true
    },

    //float primitive type
    {
        .sz = 4,
        .name = "f32",
        .prim = true
    },

    //double primitive type
    {
        .sz = 8,
        .name = "f64",
        .prim = true
    },
};