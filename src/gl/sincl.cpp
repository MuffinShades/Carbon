#include "sincl.hpp"
#include  "../lang/tokens.hpp"
#include "../msutil.hpp"

/*

#include "name" -> includes all functions from another shader
#include <lib>
#include_fn "name"@"fn" -> include specific function
#include_fn <lib>@fn

*/

struct pgrm_fn {
    std::string name;
    std::string body;
};

std::string incl_keywords[] = {
    "include",
    "include_fn"
};

const lang_info lang_inf_GLSL_incl = {
    nullptr,
    0,
    incl_keywords,
    sizeof(incl_keywords) / sizeof(std::string)
};

std::vector<pgrm_fn> get_glslShaderFunctions(std::vector<Token> pgrm_tokens) {
    i32 i, i_return;

    /*
    
    last_fn_tk_check explained

    void cool_function(){}
                      ^---

    ^ is where is checked first
    then in the smallest possible function:
        ){} must follow in order to be valid
    
    */
    const size_t nTokens = pgrm_tokens.size(), last_fn_tk_check = nTokens - 3;
    Token tk;

    for (i = 0; i < last_fn_tk_check; i++) {
        tk = pgrm_tokens[i];
        i_return = i;

        if (!tk.isTypeOf(TokenType::tok_lparen))
            continue;

        //***check the parameters*** (right side)
        //first check to make sure there even are params
        tk = pgrm_tokens[++i];

        if (tk.isTypeOf(TokenType::tok_rparen))
            goto fn_body_check;

        i32 j = i+1, alt_check = 0;

        while (j < nTokens) {
            if (alt_check & 1) {

            } else {

            }

            alt_check++;
        }

        if (false) {
        fn_check_fail:
            continue;
        }

        //check function body
        fn_body_check:



        //check left side of function (name and return type)

    } 
}

_sincl_exp AddShaderInclude(const char *src, size_t src_len, bool delete_original_src = false) {
    if (!src || src_len == 0)
        return {};

    const char* c_src = TokenGenerator::cleanCode(src, src_len);
    const size_t c_src_len = strlen(c_src);

    std::vector<Token> pgrm_tokens = TokenGenerator::genProgramTokens(c_src, c_src_len, lang_inf_GLSL_incl);

    _sincl_exp res = {
        .new_src = (char*) c_src,
        .src_len = c_src_len
    };

    i32 i;
    size_t n_tok = pgrm_tokens.size();
    Token tk;
    bool barrier = false;

    for (i = 1; i < n_tok; i++) {
        tk = pgrm_tokens[i];
        if (tk.isTypeOf(TokenType::tok_tag)) {
            barrier = true;
            continue;
        } else if (!barrier) {
            i++;
            continue;
        }

        if (tk.is("include")) {
            
        } else if (tk.is("include_fn")) {

        }

        barrier = false;
    }
}