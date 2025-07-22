#include "compiler.hpp"
#include "ast_types.hpp"
#include "lang_info.hpp"
#include "dbg_tools.hpp"
#include "util.hpp"

//removes whitespace other than spaces and comments
const char *mslang::clean_code(const char *source, size_t srcLen) {

    char *cur = (char*) source, *end = (char*) source + srcLen;
    bool slc = false, mlc = false; //bools for whether or not we in a single or multiline comment

    const char slc_cmp[] = SINGLE_LINE_COMMENT;
    const char mlcs_cmp[] = MULTI_LINE_COMMENT_START;
    const char mlce_cmp[] = MULTI_LINE_COMMENT_END;

    const size_t slc_len = strlen(slc_cmp);
    const size_t mlcs_len = strlen(mlcs_cmp);
    const size_t mlce_len = strlen(mlce_cmp);

    char *out_buffer = new char[srcLen], *cur_out = out_buffer;

    //std::cout << "Src Len: " << srcLen << std::endl;
    //std::cout << "Clean Code: ";

    int currentLine = 1;

    do {
        if (*cur == '\n') {
            slc = false;
            currentLine++;
            *cur_out++= '\n';
        }

        if (mlc && *cur == mlce_cmp[0]) {
            LENGTH_MATCH(cur, mlce_cmp, mlce_len, mlc);
            mlc = !mlc;
            if (!mlc) {
                cur++;
                continue;
            }
        }

        if (!slc && !mlc) {
            if (*cur == slc_cmp[0]) {
                LENGTH_MATCH(cur, slc_cmp, slc_len, slc);
                if (slc) {
                    cur += slc_len - 1;
                    continue;
                }
            }

            if (!slc && *cur == mlcs_cmp[0]) {
                LENGTH_MATCH(cur, mlcs_cmp, mlcs_len, mlc);
                if (mlc) {
                    cur += mlcs_len - 1;
                    continue;
                }
            }

            switch (*cur) {
                case '\t': break;
                case '\r': break;
                case '\v': break;
                default: {
                    //todo: add to result
                    char prev = *(cur - 1), next = *(cur + 1);

                    if (
                        IS_ALPHA_SYNUMERICAL(*cur) || 
                        (
                            *cur == ' ' && 
                            (
                                (IS_ALPHA_NUMERICAL(prev) && 
                                 IS_ALPHA_NUMERICAL(next)) ||
                                (IS_SYMBL(prev) &&
                                 IS_SYMBL(next))
                            )
                        )
                    )
                        *cur_out++ = *cur;
                    break;
                }
            }
        }
    } while(++cur <= end);

    *cur_out = 0x00;
    return out_buffer;
}

const char *mslang::compile(const char *source, size_t srcLen) {
    dbg.PrintSeparator();
    /*
         _
        / \
     _  | |
    / \ | |
    | | | |  _
    \ \_| | / \   Suguaro Compiler
     \__  | | |   Version 0.0
        | |_/ /
        |  __/
        | |
      _\| |/._
    
    */
    dbg.PrintColor("     _\n    / \\\n _  | |\n/ \\ | |\n| | | |  _\n\\ \\_| | / \\   Suguaro\n \\__  | | |   Version 0.0\n    | |_/ /   Copyright (c) muffinshades 2024\n    |  __/    Compiler for cacti\n    | |\n  _\\| |/._", {0, 200, 0});
    dbg.PrintSeparator();

    if (!glblLoad)
        _load_glbl_data();

    dbg.PrintSeparator();
    dbg.Log("Cleaning Code...", {255, 0, 255});
    const char *clean_source = mslang::clean_code(source, srcLen);

    if (clean_source == nullptr) {
        dbg.Error("Error: Failed to clean code!");
        return nullptr;
    }

    //std::cout << "Clean Code: " << clean_source << std::endl;

    const size_t clean_len = strlen(clean_source);

    dbg.Log("Code: ", {200, 255, 0});
    dbg.LogHex((unsigned char*)(clean_source), clean_len, {.displayChars = true});

    //get program tokens
    dbg.PrintSeparator();
    dbg.Log("Generating Program Tokens...", {255, 0, 255});
    std::vector<Token> pgrm_tokens = TokenGenerator::genProgramTokens(clean_source, clean_len);

    /*std::cout << "Program Tokens: " << std::endl;

    for (auto v : pgrm_tokens) {
        //std::cout << tok_type_strs[v.ty] << " " << v.rawValue << std::endl;
        std::cout << v.rawValue << std::endl;
    }*/

    dbg.PrintSeparator();
    dbg.Log("Generating Ast...", {255,0,255});

    token_container *tk = new token_container(pgrm_tokens);
    AstGenerator ast_gen = AstGenerator(tk);
    ast_node *ast = ast_gen.generate_ast();

    delete tk;
    delete[] clean_source;
    return nullptr;
}