#pragma once
#include <iostream>
#include <vector>
#include "ast_types.hpp"
#include "ast_nodes.hpp"
#include "lang_consts.hpp"
#include "tokens.hpp"
#include "util.hpp"

class token_container {
private:
    std::vector<Token> tokens;
    size_t pos = 0;
    const size_t nToks;
public:
    token_container(std::vector<Token> tokens) : nToks(tokens.size()) {
        this->tokens = tokens;
    }
    Token eat() {
        if (this->pos >= this->nToks) {
            this->pos = this->nToks - 1;
            return this->tokens[this->pos];
        }
        return this->tokens[this->pos++];
    }
    Token cur() {
        return this->tokens[this->pos];
    }
    Token *cur_ptr() {
        return &this->tokens[this->pos];
    }
    Token next() {
        if (this->pos+1 >= this->nToks)
            return this->tokens[this->pos];
        return this->tokens[this->pos+1];
    }
    Token prev() {
        if (this->pos <= 0)
            return this->tokens[0];
        return this->tokens[this->pos-1];
    }
    Token step_back() {
        if (this->pos <= 0)
            return this->tokens[0];
        return this->tokens[this->pos--];
    }
    Token *eat_ptr() {
        if (this->pos >= this->nToks) {
            this->pos = this->nToks - 1;
            return &this->tokens[this->pos];
        }
        return &this->tokens[this->pos++];
    }
    bool end() {
        return this->pos >= this->nToks - 1;
    }
    void tok_mini_dump() {
        const int nTokPrint = 10;
        int ds = max(0, (int)this->pos - (nTokPrint / 2));

        std::cout << "H " << ds << " " << nTokPrint << " " << this->pos - (nTokPrint / 2) << std::endl;

        for (int i = ds; i < ds + nTokPrint; i++) {
            if (i == this->pos)
                dbg.Log(this->tokens[i].rawValue+" ", {0,0,255});
            else
                dbg.Log(this->tokens[i].rawValue+" ", {255,255,255});
        }
    }
};

//used for simple error detection
struct AstScope {
    AstScope *parent = nullptr;
    std::vector<AstScope*> subScopes;
    std::vector<Token> declaredFunctions; //function and such
    std::vector<Token> declaredVars; //variables and such
    std::vector<CustomType> declaredTypes; //classes and such
    void add_var_decl(std::string name) {
        dbg.Log("Added Var To Scop: " +name);
        this->declaredVars.push_back({
            .rawValue = name
        });
    }
};

class AstGenerator {
private:
    token_container *tokens = nullptr;
    AstScope *pgrmGlobalScope = new AstScope;
    AstScope *currentScope = nullptr;
    ast_node_program *pgrm = nullptr;

    //determine if certain things exists given a token
    bool fnExists(Token t);
    bool varExists(Token t);
    bool typeExists(Token t);

    BasicType getTypeInfo(Token t);
    BasicType getPrimitiveType(std::string ty_name);
    BasicType getDeclType(std::string ty_name);
public:
    void PrintAst(ast_node *node, std::string t = "");

    AstGenerator(token_container *tokens) : tokens(tokens) {
        this->currentScope = pgrmGlobalScope;
    };
    
    //parsing functions
    ast_node *parse_primary_expr();
    ast_node *parse_mul_expr();
    ast_node *parse_add_expr();
    ast_node *parse_logic_expr();
    ast_node *parse_binary_expr();
    ast_node *generate_ast();
    ast_node *parse_fn_call();
    ast_node *parse_unary();            //unarys
    ast_node *parse_literal();
    ast_node *parse_str_literal();      //strings
    ast_node *parse_char_literal();     //characters
    ast_node *parse_numeric_literal();  //numbers
    ast_node *parse_generic_literal();  //variables and classes

    ast_node* parse_if_block();
    ast_node* parse_keyword();
    ast_node* parse_var_decl();
    ast_node *parse_block(ast_node_block* parent = nullptr);

    void free();
};