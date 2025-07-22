#pragma once
#include <iostream>
#include <vector>
#include <cstring>
#include "lang_consts.hpp"
#include "tokens.hpp"
#include "ast.hpp"

class mslang {
public:
    static const char *clean_code(const char* source, size_t srcLen);
    static const char* compile(const char* source, size_t srcLen);
};