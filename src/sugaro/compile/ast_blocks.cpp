#include "ast_util.hpp"

/**
 * 
 * for all the ast block stuff like if, while, for, block parsing, function declerations, class derclerations, enums, sub scopes
 * 
 */

ast_node* AstGenerator::parse_if_block() {
    if (!tokens->eat().is("if")) {
        dbg.Error("Cannot parse if statment if it isnt an if statement ;-;");
        return nullptr;
    }; //skip if token

    //parse condition block
    Token _err_report_token;
    if ((_err_report_token = tokens->cur()).ty != tok_lparen) {
        dbg.Error("Unexpected token \""+_err_report_token.rawValue+"\" on line "+std::to_string(_err_report_token.src.lineNumber)+". Expected a left parenthesis.");
        return nullptr;
    }


    //get condition
    ast_node* condition = this->parse_binary_expr();

    if (!condition) {
        dbg.Error("Failed to read if condition...");
        return nullptr;
    }

    //parse block
    ast_node *if_body = this->parse_block();

    if (!if_body) {
        dbg.Error("failed to read if body");
        return nullptr;
    }

    //create main node
    ast_node_if *if_node = new ast_node_if((ast_node_binary_exp*)condition, if_body);

    //check for else and else if statements
    ast_node_if *curIf = if_node;

    Token else_start;
    while ((else_start = tokens->cur()).isTypeOf(tok_keyword) && else_start.is("else")) {
        tokens->eat(); //skip the else
        
        ast_node *else_node = this->parse_block();

        if (!else_node) {
            free_node(if_node);
            return nullptr;
        }

        curIf->else_block = else_node;

        //check for another else if and add one if necessary
        if (else_node->ty != AST_IF_STATEMENT)
            break;
        else
            curIf = (ast_node_if*) else_node;
    }

    return if_node;
}

//TODO: handle cases where the token is {
ast_node *AstGenerator::parse_block(ast_node_block* parent) {
    bool r_brk = false;

    do {
        Token tok = this->tokens->cur(); //based on current token so no step back needed

        switch (tok.ty) {
            case tok_keyword:
                this->parse_keyword();
                break;
            case tok_literal: {
                ast_node *tg = this->parse_generic_literal();

                if (!tg) {
                    //this->free();
                    return nullptr;
                }

                //we will end up double parsing a literal but uh oh well
                //wait ima just store the parsed data in the token
                if (tg->ty == AST_CLASS_REF) {
                    this->tokens->step_back();
                    ast_node *vdcl = this->parse_var_decl();

                    if (!vdcl) {
                        dbg.Error("Error when parsing var decleration! Last: "+tok.rawValue+"  Cur: "+this->tokens->next().rawValue);
                        goto esc;
                    }

                    if (parent)
                        parent->add(vdcl);
                    else
                        return vdcl;

                    dbg.Log("Added VAR");
                } 

                dbg.Log("Parsed Var");
                break;
            }
            case tok_operator: {
                dbg.Log("Tok op??");
                this->tokens->tok_mini_dump();
                this->tokens->eat();
                break;
            }

            case tok_pgrm_end: {
                dbg.Log("Reached program end...");
                goto esc;
                break;
            }

            case tok_semi: {
                this->tokens->eat();
                break;
            }

            /**
             *
             * LBracket case
             * 
             * results in a new scope being created
             * 
             */
            case tok_lbrack: {
                if (!parent)
                    parent = new ast_node_block();
                else //else simply a sub-scope that needs to be parsed
                    this->parse_block(nullptr);
            }

            /**
             * 
             * RBracket case
             * 
             * reached end of scope
             * if
             * 
             */
            case tok_rbrack: {
                r_brk = true;
                break;
            }

            default: {
                //invalid token throw error or something
                dbg.Log("Invalid Tok: "+this->tokens->cur().rawValue, {255,0,0});
                this->tokens->eat();
                return nullptr;
            }
        }

        //debugging
        if (false) {
        esc:
            dbg.Log("ast done");
            break;
        }

    /*
    End conditions are
        Ending token found -> end of file
        no parent -> since statement parse
     */
    } while (!(this->tokens->end() || !parent || r_brk));

    return parent;
}