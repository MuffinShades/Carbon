#pragma once
#include <iostream>

const std::string operators[] = {
    "+",
    "-",
    "*",
    "/",
    "**",
    "**=",
    "+=",
    "-=",
    "*=",
    "/=",
    "++",
    "--",
    "%",
    "%=",
    "|",
    "&",
    "^",
    ">>",
    "<<",
    ">>>",
    "<<<"
    "~",
    "|=",
    "&=",
    "^=",
    ">>=",
    "<<=",
    ">>>=",
    "<<<=",
    "&&",
    "||",
    "!",
    "!=",
    "==",
    ">",
    ">=",
    "<",
    "<="
};

//unary typing
enum unary_type {
    _utype_none,
    _utype_left, //left hand only unarys like ~, !
    _utype_right, //right hand only unarys like +=, -=
    _utype_both //can be either unarys like --, ++
};

//for now just holes a unary type but later this will hold more probably
struct op_descriptor {
    unary_type uty = _utype_none;
};

//descriptions for each operator
const op_descriptor op_desc[] = {
    {}, // +
    {}, // -
    {}, // *
    {}, // /
    {}, // **
    {.uty = _utype_right}, // **=
    {.uty = _utype_right}, // +=
    {.uty = _utype_right}, // -=
    {.uty = _utype_right}, // *=
    {.uty = _utype_right}, // /=
    {.uty = _utype_both}, // ++
    {.uty = _utype_both}, // --
    {}, // %
    {.uty = _utype_right}, // %=
    {}, // |
    {}, // &
    {}, // ^
    {}, // >>
    {}, // <<
    {}, // >>>
    {}, // <<<
    {.uty = _utype_left}, // ~
    {.uty = _utype_right}, // |=
    {.uty = _utype_right}, // &=
    {.uty = _utype_right}, // ^=
    {.uty = _utype_right}, // >>=
    {.uty = _utype_right}, // <<=
    {.uty = _utype_right}, // >>>=
    {.uty = _utype_right}, // <<<=
    {}, // &&
    {}, // ||
    {.uty = _utype_left}, // !
    {}, // !=
    {}, // ==
    {}, // >
    {}, // >=
    {}, // <
    {}  // <=
};

/*const std::string comparators[] = {
    "!=",
    "==",
    ">",
    ">=",
    "<",
    "<="
};*/

const std::string keywords[] = {
    "for",
    "while",
    "if",
    "else",
    "switch",
    "const",
    "class",
    "func",
    "case",
    "try",
    "catch",
    "goto",
    "break",
    "continue",
    "return",
    "enum",
    "null",
    "encap"
};

enum keyword {
    keyword_for,
    keyword_while,
    keyword_if,
    keyword_else,
    keyword_switch,
    keyword_const,
    keyword_class,
    keyword_func,
    keyword_case,
    keyword_try,
    keyword_catch,
    keyword_goto,
    keyword_break,
    keyword_continue,
    keyword_return,
    keyword_enum,
    keyword_null
};