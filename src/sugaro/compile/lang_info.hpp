#pragma once
#include "dbg_tools.hpp"

#define SINGLE_LINE_COMMENT "//"
#define MULTI_LINE_COMMENT_START "/*"
#define MULTI_LINE_COMMENT_END "*/"

#define GET_MATCH_SIZE(buf_a, buf_b, pos) \
    do {} while(*((buf_a) + (pos)) == *((buf_b) + (pos)++));
#define LENGTH_MATCH(buf_a, buf_b, len, out) \
    size_t pos = 0; \
    GET_MATCH_SIZE((buf_a), (buf_b), pos); \
    (out) = pos > (len);
#define IN_RANGE(v, min, max) ((v) >= (min) && (v) <= (max))
#define IS_ALPHA_NUMERICAL(val) (IS_NUMERICAL(val) || IS_ALPHA(val))
#define IS_ALPHA_SYNUMERICAL(val) (IN_RANGE(val, '!', '~'))
#define IS_ALPHA(val) (IN_RANGE(val, 'A', 'Z') || IN_RANGE(val, 'a', 'z') || (val) == '_')
#define IS_SYMBL(val) (!(IS_ALPHA_NUMERICAL(val)) && (val) != ' ')
#define IS_TOK_SYMBL(val) (IS_SYMBL(val) && ((val) != ';' && (val) != ':' && (val) != '\'' && (val) != '"' && !IS_BRACKET(val) && (val) != ',' && (val) != '.' && (val) != '`' && (val) != '?' && (val) != '@' && (val) != '#' && (val) != '_'))
#define IS_BRACKET(val) ((val) == '{' || (val) == '}' || (val) == '[' || (val) == ']' || (val) == '(' || (val) == ')')
#define IS_NUMERICAL(val) ((val) >= '0' && (val) <= '9')

//TODO: MOVE THIS TO A LANGUAGE INFO CLASS
static size_t *comp_szs = nullptr; //size of every comparator
static size_t comp_size_max = 0;
static size_t n_operators = 0, n_comparators = 0, n_keywords = 0;
static bool glblLoad = false;

static inline void _load_glbl_data() {
    for (auto& x : operators) n_operators++;
    //for (auto& x : comparators) n_comparators++;
    for (auto& x : keywords) n_keywords++;
    glblLoad = true;

    dbg.Log("Loaded Global Dat | no: "+std::to_string(n_operators)+" | nk: "+std::to_string(n_keywords), {0,255,255});
}