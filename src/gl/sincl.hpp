//shader include operations
/*

Copyright Lambdana Software 2025 All Rights Reserved
Programmed by muffinshades

#include "name" -> includes all functions from another shader
#include <lib>
#include_fn "name"@fn -> include specific function
#include_fn <lib>@fn

*/
#include <iostream>
#include "../lang/tokens.hpp"

struct _sincl_exp {
    char *new_src = nullptr;
    size_t src_len = 0;
};

class _sincl_src_finder;

class Sincl {
private:
    static bool ihistory_enable;
    static void enable_ihistory() {
        ihistory_enable = true;
    }
    static void disable_ihistory() {
        ihistory_enable = false;
    }
public:
    struct Options {
        bool delete_original_src = false;
        bool remove_version = false;
    };
    static _sincl_exp AddShaderInclude(const char *src, size_t src_len, Sincl::Options settings, void *__ihistory = nullptr);
    friend class _sincl_src_finder;
};