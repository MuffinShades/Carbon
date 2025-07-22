#include "ast_util.hpp"

/*

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

*/

const std::string typeNames[] = {
    "Unknown",
    "Program",
    "Num Literal",
    "Float Literal",
    "String Literal",
    "For Loop",
    "While Loop",
    "Block",
    "Func Decl",
    "If",
    "Binary Expr",
    "Var Decl",
    "Class Decl",
    "Func Call", 
    "Ternerary",
    "Include",
    "Unary",
    "Enum Decl",
    "Var ref",
    "Class ref"
};

const enum tcn {
    tcn_num
};

const Color typeColors[] = {
    {0,0,255}
};

//Print Ast
//
//prints tree for debug purposess
void AstGenerator::PrintAst(ast_node *node, std::string t) {
    if (!node) return;
    std::cout << t << "Node: " << typeNames[node->ty] << std::endl;
    //t += '\t';

    switch (node->ty) {
        case AST_PROGRAM: {
            std::cout << t << "Program: " << std::endl;
            ast_node_program* p = (ast_node_program*) node;
            for (auto &n : p->body)
                PrintAst(n, t+'\t');
            break;
        }
        case AST_NUMBER_LITERAL: {
            std::cout << t << "Value: " << ((ast_node_numeric_literal*) node)->val << std::endl;
            break;
        }
        case AST_BINARY_EXPRESSION: {
            ast_node_binary_exp *n = (ast_node_binary_exp*) node;
            std::cout << t << "Operator: " << operators[n->op] << std::endl;
            std::cout << t << "Left: " << std::endl;
            PrintAst(n->left, t+'\t');
            std::cout << t << "Right: " << std::endl;
            PrintAst(n->right, t+'\t');
            break;
        }
        case AST_VAR_DECLERATION: {
            ast_node_var_decl *n = (ast_node_var_decl*) node;
            std::cout << t << "Name: " << n->name << std::endl;
            std::cout << t << "Type: " << n->vty.name << std::endl;
            std::cout << t << "Assign Value: " << std::endl;
            PrintAst(n->asign_val, t+'\t');
            break;
        }
        case AST_IF_STATEMENT: {
            ast_node_if *n = (ast_node_if*) node;
            
            break;
        }
        default: std::cout << t << "Unknown Node Data" << std::endl;
    }
}