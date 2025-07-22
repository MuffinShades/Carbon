#pragma once
#include <iostream>
#include <vector>
#include "lang_consts.hpp"

enum TokenType {
    tok_notype,
    tok_literal,
    tok_operator,
    tok_semi,
    tok_lparen,
    tok_rparen,
    tok_lbrack,
    tok_rbrack,
    tok_lsqbrack,
    tok_rsqbrack,
    tok_keyword,
    tok_colon,
    tok_at,
    tok_comma,
    tok_num_literal,
    tok_decimal,
    tok_pgrm_end
};

const std::string tok_type_strs[] = {"NOTYPE", "LITERAL", "OPP", "LNEND", 
                                     "LPAREN", "RPAREN", "LBRACK", "RBRACK", 
                                     "LSQBRACK", "RSQBRACK", "KEYWORD", "COLON",
                                     "@", "COMMA"};

struct psinf {
    bool isType, isVar, isFn;
};

struct SrcMap {
    int lineNumber = -1;
    int linePos = -1;
    std::string file = "";
};

struct Token {
    enum TokenType ty;
    std::string rawValue;
    int mode = 0;
    int64_t rawiValue = 0;
    int enum_id = -1;
    uint64_t raw_checksum; //used for quick search / check of a string to see if it matches an existing one
    size_t len;
    SrcMap src;
    psinf inf;
    bool is(std::string v);
    bool isTypeOf(TokenType ty);
};

class TokenGenerator {
public:
    static std::vector<Token> genProgramTokens(const char* source, size_t srcLen);
};