#pragma once
#include <iostream>
#include <vector>
#include "dbg_tools.hpp"
#include "ast_types.hpp"

enum ast_type {
    AST_UNKNOWN,
    AST_PROGRAM,
    AST_NUMBER_LITERAL,
    AST_FLOAT_LITERAL,
    AST_STRING_LITERAL,
    AST_FOR_LOOP,
    AST_WHILE_LOOP,
    AST_BLOCK,
    AST_FUNC_DECLERATION,
    AST_IF_STATEMENT,
    AST_BINARY_EXPRESSION,
    AST_VAR_DECLERATION,
    AST_CLASS_DECLERATION,
    AST_FUNC_CALL,
    AST_TERNERARY,
    AST_INCLUDE,
    AST_UNARY,
    AST_ENUM_DECLERATION,
    AST_VAR_REF,
    AST_CLASS_REF
};

struct ast_return_info {
    int returnCode = 0;
    bool error = false;
};

class ast_node {
public:
    ast_return_info r_inf;
    ast_type ty;
    ast_node(ast_type ty = AST_UNKNOWN) : ty(ty) {};
};

class ast_node_numeric_literal : public ast_node {
public:
    uint64_t val;
    bool sign;
    ast_node_numeric_literal() : ast_node(AST_NUMBER_LITERAL) {

    }
};

class ast_node_float_numeric_literal : public ast_node {
public:
    double val;
    bool f32_bit = true;
    ast_node_float_numeric_literal() : ast_node(AST_FLOAT_LITERAL) {

    }
};

class ast_node_string_literal : public ast_node {
public:
    std::string val;
    ast_node_string_literal() : ast_node(AST_STRING_LITERAL) {

    }
};


//todo
class ast_node_for : public ast_node {
public:
    ast_node_for() : ast_node(AST_FOR_LOOP) {

    }
};

class ast_node_while : public ast_node {
public:
    ast_node *condition = nullptr;
    ast_node *body;
    ast_node_while() : ast_node(AST_WHILE_LOOP) {

    }
};

class ast_node_block : public ast_node {
public:
    std::vector<ast_node *> body;
    ast_node_block() : ast_node(AST_BLOCK) {

    }
    void add(ast_node *n) {
        if (n != nullptr)
            this->body.push_back(n);
        else
            dbg.Warn("Attempted to add a null node to program ast_node");
    }
    void free() {
        for (auto* i : this->body)
            delete i;
        this->body.clear();
    }
};

class ast_node_func : public ast_node {
public:
    std::string name;
    std::vector<ast_node *> params;
    ast_node *body;
    ast_node_func() : ast_node(AST_FUNC_DECLERATION) {

    }
};

class ast_node_binary_exp : public ast_node {
public:
    ast_node *left = nullptr, *right = nullptr; //accepted types: literal, numeric literal, binary exp
    int op = -1;
    ast_node_binary_exp() : ast_node(AST_BINARY_EXPRESSION) {

    }
};

class ast_node_if : public ast_node {
public:
    ast_node_binary_exp *condition = nullptr;
    ast_node *else_block = nullptr; //accepted types: block, if
    ast_node *body = nullptr;
    ast_node_if(ast_node_binary_exp *condition = nullptr, ast_node *body = nullptr, ast_node *else_block = nullptr) : ast_node(AST_IF_STATEMENT) {
        this->condition = condition;
        this->body = body;
        this->else_block = else_block;
    }
};

class ast_node_var_decl : public ast_node {
public:
    std::string name = "";
    ast_node *asign_val = nullptr;
    BasicType vty;
    ast_node_var_decl() : ast_node(AST_VAR_DECLERATION) {

    }
};

class ast_node_class_decl : public ast_node {
public:
    std::string name;
    ast_node *body = nullptr;
    ast_node_class_decl() : ast_node(AST_CLASS_DECLERATION) {

    }
};

class ast_node_func_call : public ast_node {
public:
    std::string fn;
    std::vector<ast_node *> params;
    ast_node_func_call() : ast_node(AST_FUNC_CALL) {

    }
};

class ast_node_ternerary : public ast_node {
public:
    ast_node_binary_exp *condition = nullptr;
    //left = true, right = false;
    ast_node *left = nullptr;
    ast_node *right = nullptr;
    ast_node_ternerary() : ast_node(AST_TERNERARY) {

    }
};

class ast_node_include : public ast_node {
public:
    ast_node_include() : ast_node(AST_INCLUDE) {

    }
};

//unary types
enum UnaryType {
    UnaryType_Expr = 0,
    UnaryType_Inline_Left = 2,
    UnaryType_Inline_Right = 3
};

//macro to get whether or not the unary is inline
#define UNARY_TYPE_IS_INLINE(uty) (((uty) & 0x10) >> 1)

//unary class
class ast_node_unary : public ast_node {
public:
    ast_node *exp; //expression that will modify the mod value
    ast_node *mod_val; //value to modified by unary
    int op = -1;
    UnaryType ty;
    ast_node_unary() : ast_node(AST_UNARY) {

    }
};

//enums
class ast_node_enum : public ast_node {
public:
    ast_node_enum() : ast_node(AST_ENUM_DECLERATION) {

    }
};

class ast_node_program : public ast_node_block {
public:
    ast_node_program() : ast_node_block() {
        this->ty = AST_PROGRAM;
    }
};

class ast_node_var_ref : public ast_node {
public:
    std::string ref_var;
    ast_node_var_ref() : ast_node(AST_VAR_REF) {

    }
};

class ast_node_class_ref : public ast_node {
public:
    std::string ref_class;
    ast_node_class_ref() : ast_node(AST_CLASS_REF) {

    }
};

template <class _Ty> static _Ty deduceNode(ast_node *n) {
    if (!n) return nullptr;

    switch (n->ty) {
    case AST_BINARY_EXPRESSION:
        return (ast_node_binary_exp*) n;
    case AST_BLOCK:
        return (ast_node_block*) n;
    case AST_CLASS_DECLERATION:
        return (ast_node_class_decl*) n;
    case AST_CLASS_REF:
        return (ast_node_class_ref*) n;
    case AST_ENUM_DECLERATION:
        return (ast_node_enum*) n;
    case AST_FLOAT_LITERAL:
        return (ast_node_float_numeric_literal*) n;
    case AST_FOR_LOOP:
        return (ast_node_for*) n;
    case AST_FUNC_CALL:
        return (ast_node_func_call*) n;
    case AST_FUNC_DECLERATION:
        return (ast_node_func*) n;
    case AST_IF_STATEMENT:
        return (ast_node_if*) n;
    case AST_INCLUDE:
        return (ast_node_include*) n;
    case AST_NUMBER_LITERAL:
        return (ast_node_numeric_literal*) n;
    case AST_PROGRAM:
        return (ast_node_program*) n;
    case AST_STRING_LITERAL:
        return (ast_node_string_literal*) n;
    case AST_TERNERARY:
        return (ast_node_ternerary*) n;
    case AST_UNARY:
        return (ast_node_unary*) n;
    case AST_VAR_DECLERATION:
        return (ast_node_var_decl*) n;
    case AST_VAR_REF:
        return (ast_node_var_ref*) n;
    case AST_WHILE_LOOP:
        return (ast_node_while*) n;
    case AST_UNKNOWN:
    default:
        return n;
    }
}