//shader include operations
/*

#include "name" -> includes all functions from another shader
#include <lib>
#include_fn "name"@"fn" -> include specific function
#include_fn <lib>@fn

*/
#include <iostream>
#include "../lang/tokens.hpp"

struct _sincl_exp {
    char *new_src = nullptr;
    size_t src_len = 0;
};

class Sincl {
public:
    _sincl_exp AddShaderInclude(const char *src, size_t src_len, bool delete_original_src = false);
};