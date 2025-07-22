#include "ast_util.hpp"

//keywords
ast_node* AstGenerator::parse_keyword() {
    Token tok = this->tokens->cur();

    dbg.Log("Parsing Keyword: "+tok.rawValue, {255,255,128});

    switch (tok.enum_id) {
        case keyword_if: {
            dbg.PrintColor("Parsing If...", {255,0,0});
            ast_node *if_block = this->parse_if_block();
            break;
        }
        default: {
            //throw error
            dbg.Error("parse_keyword error : Invalid keyword -> "+tok.rawValue+" | Enum: "+std::to_string(tok.enum_id));
            return nullptr;
        }
    }

    return nullptr;
}

ast_node *AstGenerator::generate_ast() {
    this->pgrm = new ast_node_program;

    //dbg.Log("Gen Start at token: "+tok.rawValue);

    this->parse_block(this->pgrm);

    PrintAst(this->pgrm);

    return nullptr;
}