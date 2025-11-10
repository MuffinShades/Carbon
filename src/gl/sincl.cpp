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

        i32 j = i+1, alt_check = 0;

        if (!tk.isTypeOf(TokenType::tok_lparen))
            continue;

        //***check the parameters*** (right side)
        //first check to make sure there even are params
        tk = pgrm_tokens[++i];

        if (tk.isTypeOf(TokenType::tok_rparen))
            goto fn_body_check;

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

    //this stuff is used a lil later in the function
    enum class sector_src_type {
        unknown,
        intra, //source code itself
        inter  //within other sources / libraries
    };

    struct output_sector {
        sector_src_type ty = sector_src_type::unknown;
        size_t sz;
        size_t offset;
    };

    //
    const char* c_src = TokenGenerator::cleanCode(src, src_len);
    const size_t c_src_len = strlen(c_src);

    std::vector<Token> pgrm_tokens = TokenGenerator::genProgramTokens(c_src, c_src_len, lang_inf_GLSL_incl);

    _sincl_exp res = {
        .new_src = (char*) c_src,
        .src_len = c_src_len
    };

    i32 i, j;
    size_t n_tok = pgrm_tokens.size(),
           last_proc_tok = n_tok + 3; //when processing and include statement, there must be at least 3 tokens left for the forms "asdf" and <asdf>
    size_t prev_chunk_end = 0;
    Token tk;
    bool barrier = false;

    //skeleton of the output file
    std::vector<output_sector> output_skeleton;
    size_t output_sz = 0;

    for (i = 0; i < last_proc_tok; i++) {
        tk = pgrm_tokens[i];
        if (tk.isTypeOf(TokenType::tok_tag)) {
            barrier = true;
            continue;
        } else if (!barrier) {
            i++;
            continue;
        }

        if (i < 1) continue; //simple barrier to prevent possibly subtracting 1 from an i val of 0 below when computing intra chunk size

        if (tk.is("include")) {
            //create a new output sector

            //intra chunk
            const size_t intra_chunk_sz = (i - 1) - prev_chunk_end;
            output_sector intra_chunk = {
                .ty = sector_src_type::intra,
                .sz = intra_chunk_sz,
                .offset = prev_chunk_end
            };
            output_skeleton.push_back(intra_chunk);

            //inter chunk
            tk = pgrm_tokens[++i];

            switch (tk.ty) {
            //"
            case TokenType::tok_double_quote: {

                break;
            }
            //<
            case TokenType::tok_operator: {

                break;
            }
            default: {
                std::cout << "sincl error! invalid include format" << std::endl;
                delete[] c_src;
                return {};
            }
            };

            //compute new prev_chunk_end
        } else if (tk.is("include_fn")) {
            //create a new output sector

            //intra chunk
            const size_t intra_chunk_sz = (i - 1) - prev_chunk_end;
            output_sector intra_chunk = {
                .ty = sector_src_type::intra,
                .sz = intra_chunk_sz,
                .offset = prev_chunk_end
            };
            output_skeleton.push_back(intra_chunk);

            //inter chunk

            //compute new prev_chunk_end
        }

        barrier = false;
    }

    //convert the skeleton into source code
    //add 1 for the nul terminator
    const size_t output_wterm_sz = output_sz + 1;
    char *o_src = new char[output_wterm_sz];
    ZeroMem(o_src, output_wterm_sz);

    size_t w_pos = 0; //write position in the output

    for (const auto sec : output_skeleton) {

    }

    //pack output source into a sincl_exp and export it

}