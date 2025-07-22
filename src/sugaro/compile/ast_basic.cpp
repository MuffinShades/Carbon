#include "ast_util.hpp"
/**
 * 
 * For all the basic ast functions like arithmatic, and literal parsing
 * 
 * says basic but unironically will probably be by
 * far the largest file since it basically includes
 * all the core architecture and functions for ast
 * generation
 * 
 */

//TODO: change the simple delete frees into calls to a free function that will properly handle the freeing of sub nodes

struct ExprContext {
    size_t exprLen = 0;
    size_t *depthMap = nullptr;
};

/**
 * 
 * Functions to check if the various operations are what they are
 * 
 * example, check for +, -, *, /, and % but ommit unary operators
 * 
 */

bool isAddOp(Token t) {
    return (t.len == 1 && (t.rawValue[0] == '+' || t.rawValue[0] == '-'));
}

bool isMulOp(Token t) {
    return (t.len == 1 && (t.rawValue[0] == '*' || t.rawValue[0] == '/' || t.rawValue[0] == '%'));
}

bool isLogicOp(Token t) {
    return (
        (t.len == 1 && (t.rawValue[0] == '&' || t.rawValue[0] == '|' || t.rawValue[0] == '^')) ||
        (t.is(">>") || t.is("<<") || t.is(">>>") || t.is("<<<"))
    );
}

/**
 * 
 * AstGenerator::parse_primary_expr
 * 
 * Will parse a primary expression such as
 * something in parenthesis and literals
 * 
 * -> uses parse literal function so dosent
 * actually pare literals
 * 
 * also handldes parsing for casts
 * 
 */
ast_node *AstGenerator::parse_primary_expr() {
    Token tk = tokens->cur();

    dbg.Log("Got primary token: "+tk.rawValue);

    switch (tk.ty) {

        //parse a left parenthesis
        case tok_lparen: {
            this->tokens->eat();
            ast_node *res = this->parse_binary_expr();
            Token e_tok;
            if ((e_tok = this->tokens->eat()).ty != tok_rparen) {
                dbg.ThrowUnexpectedToken(")", tk);
                return nullptr;
            }
            std::cout << "PRIMARY TOKEN END: " << e_tok.rawValue << std::endl;
            return res;
        }

        //unarys
        case tok_operator:
        case tok_literal:
            return this->parse_unary();

        //numeric literal
        case tok_num_literal:
            return this->parse_literal();

        //
        default: {
            dbg.ThrowUnexpectedToken(tk);
            return nullptr;
        }
    }
}  

/**
 * 
 * AstGenerator::parse_generic_literal
 * 
 * one of the final steps in parsing a literal
 * will basically parse a generic word such as
 * a variable or class and then will return the
 * generated node
 * 
 * One of 2 Alternative final step in parse_literal
 * for non numeric literalls
 * 
 */

ast_node *AstGenerator::parse_generic_literal() {
    Token *target = this->tokens->cur_ptr();

    if (this->typeExists(*target)) {
        ast_node_class_ref *res = new ast_node_class_ref();
        res->ref_class = target->rawValue;
        target->inf.isType = true; //inf stuff
        this->tokens->eat(); //skip variable token
        return res;
    }

    //TODO: fix the var exists thing
    if (/*this->varExists(*target)*/1) {
        ast_node_var_ref *res = new ast_node_var_ref();
        res->ref_var = target->rawValue;
        target->inf.isVar = true; //inf stuff
        this->tokens->eat(); //skip variable token
        return res;
    }

    dbg.Error("Unknown literal \""+target->rawValue+"\" on line "+std::to_string(target->src.lineNumber));
    return nullptr;
}

/**
 * 
 * AstGenerator::parse_fn_call
 * 
 */

ast_node *AstGenerator::parse_fn_call() {
    return nullptr;
}

/**
 * 
 * AstGenerator::parse_literal
 * 
 * parses next literal in generator's token stream
 * returns a pointer to a new node which will have all 
 * the data about what kinda of literal was parsed
 * 
 * Will Parse:
 * 
 * numeric
 * fn calls
 * lambda declares
 * var ref
 * class types
 * 
 * TODO: scientific notation
 * TODO: numeric escape sequences: 0x001, 08001, and 0b001 
 * TODO: string literals
 * TODO: char literalls
 * TODO: string and char escape: \u023, \n, \r, \t etc.
 * TODO: split it into various functions like numeric, generic, etc. and have this one handle any
 */
ast_node *AstGenerator::parse_literal() {
    Token t = this->tokens->eat(), nxt = this->tokens->cur();
    
    bool neg_int = false;

    //token we got isnt a literal
    if (t.ty != tok_num_literal && t.ty != tok_literal) {

        //check in case of negative number
        if (
            t.isTypeOf(tok_operator) &&
            t.is("-") &&
            nxt.isTypeOf(tok_num_literal)
        ) {
            neg_int = true;
            t = this->tokens->eat();
            nxt = this->tokens->cur();
            goto num_parse;
        }

        //throw unexpected token error since we have an unknown type
        dbg.ThrowUnexpectedToken("a literal", t);
        return nullptr;
    }

    //numeric literals
    if (t.ty == tok_num_literal) {
num_parse: //for jumping when we have a negative number parsing
        uint64_t base = (uint64_t) t.rawiValue;

        //floating point number
        if (nxt.isTypeOf(tok_decimal)) {
            ast_node_float_numeric_literal *res = new ast_node_float_numeric_literal();

            //get decimal value
            this->tokens->eat(); // skip decimal
            Token dec_t = this->tokens->eat();

            if (!dec_t.isTypeOf(tok_num_literal)) {
                dbg.ThrowUnexpectedToken("numeric literal", dec_t);
                delete res;
                return nullptr;
            }

            uint64_t dec = dec_t.rawiValue;

            //set value
            res->val = ((double) base + convertToDecimal(dec)) * ((double)neg_int * 2.0f - 1.0f);

            //determine if float
            if (this->tokens->next().isTypeOf(tok_literal) && this->tokens->next().is("f")) {
                this->tokens->eat(); //skip "f"
            } else
                res->f32_bit = false;

            return res;
        }

        //integer
        ast_node_numeric_literal *res = new ast_node_numeric_literal();
        res->val = base;
        res->sign = !neg_int;
        return res;
    }

    //other literals
    switch (nxt.ty) {
        //fn call
        case tok_lparen: {
            this->tokens->step_back();
            return this->parse_fn_call();
        }
        
        default: {
            this->tokens->step_back();
            return this->parse_generic_literal();
        }
    }

    return nullptr;
}

//TODO: MAKE THIS SAFER
bool isExprUnaryOp(Token t) {
    return (t.rawValue[t.rawValue.length()-1] == '=');
}

ast_node *AstGenerator::parse_unary() {
    auto ty = tokens->cur().ty;

    switch (ty) {
    case tok_operator: {
        //left side
        ast_node_unary *U = new ast_node_unary();
        U->ty = UnaryType_Inline_Left;
        U->mod_val = this->parse_unary();
        U->op = tokens->cur().enum_id; //WARNING: this could cause issue if enum_id isn't the type of operator the unary is
        return U;
    }
    default: {
        //inline right or expr
        if (ty != tok_literal) {
            dbg.ThrowUnexpectedToken(tokens->cur());
            return nullptr;
        }

        //get left hand side
        ast_node *left = this->parse_literal();

        if (!left) {
            dbg.Error("Invalid left hand unary side!");
            return nullptr;
        }

        //right sides
        Token next = tokens->next();

        if (!isExprUnaryOp(next)) { //no expr unary
            if (next.isTypeOf(tok_operator)) { //non expr unary
                ast_node_unary* _ures = new ast_node_unary();

                _ures->ty = UnaryType_Inline_Right;
                _ures->op = next.enum_id;
                _ures->mod_val = left;

                return _ures;
            } else //just a literal
                return left;
        } else {
            //create unary thingy
            ast_node_unary *_ures = new ast_node_unary();

            _ures->mod_val = left;
            _ures->op = next.enum_id;
            _ures->exp = this->parse_binary_expr();
            _ures->ty = UnaryType_Expr;

            if (!_ures->mod_val) {
                dbg.Error("Failed to parse right hand side of unary!");
                free_node(left);
                free_node(_ures);
                return nullptr;
            }

            return _ures;
        }
        }
    }

    return nullptr;
}

ast_node *AstGenerator::parse_literal() {
    Token next = tokens->next();

    switch (next.ty) {
        case tok_literal:
            switch (next.rawValue[0]) {
                case '"': 
                    return this->parse_str_literal();
                case '\'':
                    return this->parse_char_literal();
                default:
                    return this->parse_generic_literal();
            }
            break;
        case tok_num_literal:
            return this->parse_numeric_literal();
        default:
            dbg.Warn("Unknown Token: "+next.rawValue+" when parsing literal!");
            return nullptr;
    }
}

ast_node *AstGenerator::parse_char_literal() {

}

ast_node *AstGenerator::parse_str_literal() {

}

/**
 * 
 * AstGenerator::parse_binary_expr
 * 
 * basically just starts the procedure that will parse
 * binary expressions. Basically just a call to parse_logic_expr
 * except it's its own function to make parsing binary
 * expressions more logical
 * 
 */
ast_node *AstGenerator::parse_binary_expr() {
    return this->parse_logic_expr();
}

/**
 * 
 * AstGenerator::parse_logic_expr
 * 
 * first step in parsing a binary expression
 * 
 * Operations: and, or, xor, rsh, lsh
 * 
 */
ast_node *AstGenerator::parse_logic_expr() {
    ast_node_binary_exp *res = new ast_node_binary_exp();

    res->left = parse_add_expr();
    bool _first = true;
    Token cur;
    
    while (
        isLogicOp 
    ) {
        if (_first)
            _first = !_first;
        else {
            //create new node and set left to old node
            ast_node_binary_exp *o_res = res;
            res = new ast_node_binary_exp();
            res->left = o_res;
        }

        res->op = getOperatorId(cur.rawValue);
        tokens->eat(); //skip operator

        //get the right
        res->right = parse_add_expr();
    }

    if (!res->left || !res->right) {
        if (_first) {
            ast_node *rl = res->left;
            delete res;
            return rl;
        }
        //wait report something went wrong first
        dbg.Warn("[AST] Something went wrong while parsing logic expression?");
        return nullptr; //something went wrong
    }

    return res;
}

ast_node *AstGenerator::parse_add_expr() {
    ast_node_binary_exp *res = new ast_node_binary_exp();

    res->left = parse_mul_expr();
    bool _first = true;
    Token cur;

    std::cout << "ADD OP: " << tokens->cur().rawValue << std::endl;
    
    while (
        isAddOp(cur = tokens->cur())
    ) {
        if (_first)
            _first = !_first;
        else {
            //create new node and set left to old node
            ast_node_binary_exp *o_res = res;
            res = new ast_node_binary_exp();
            res->left = o_res;
        }

        res->op = getOperatorId(cur.rawValue);
        Token t_ok = tokens->eat(); //skip operator

        std::cout << "Skipped Token: " << t_ok.rawValue << std::endl;

        //get the right
        res->right = this->parse_mul_expr();
    }

    if (!res->left || !res->right) {
        if (_first) {
            ast_node *rl = res->left;
            delete res;
            return rl;
        }
        //wait report something went wrong first
        dbg.Warn("[AST] Something went wrong while parsing add expression?");
        return nullptr; //something went wrong
    }

    return res;
}

ast_node *AstGenerator::parse_mul_expr() {
    dbg.Log("Parsing mul expr...");
    ast_node_binary_exp *res = new ast_node_binary_exp();

    //--debug--//
    std::vector<Token> operator_collection;

    //parse left hand side
    res->left = this->parse_primary_expr();
    bool _first = true;
    Token cur;
    
    while (
        isMulOp(cur = tokens->cur())
    ) {
        if (_first)
            _first = !_first;
        else {
            //create new node and set left to old node
            ast_node_binary_exp *o_res = res;
            res = new ast_node_binary_exp();
            res->left = o_res;
        }

        res->op = getOperatorId(cur.rawValue);
        Token opSkip = tokens->eat(); //skip operator
        operator_collection.push_back(opSkip);

        //get the right
        res->right = parse_primary_expr();
    }

    //--debug--//
    dbg.PrintColor("--Parsed "+std::to_string(operator_collection.size())+" Mul Expr--", {0,0,0}, {255,0,255});
    dbg.PrintColor("Operators: ", {200,200,200});
    for (Token t : operator_collection)
        dbg.PrintColor("\t"+t.rawValue, {255,255,0});
    dbg.PrintSeparator();

    if (!res->left || !res->right) {
        if (_first) {
            ast_node *rl = res->left;
            delete res;
            return rl;
        }
        //wait report something went wrong first
        dbg.Warn("[AST] Something went wrong while parsing multiply expression?");
        return nullptr; //something went wrong
    }

    return res;
}

ast_node* AstGenerator::parse_var_decl() {
    dbg.Log("Parsing var decleration...");
    Token *type_token = this->tokens->eat_ptr();

    this->tokens->tok_mini_dump();

    //make sure token is a type
    if (
        !type_token->inf.isType &&
        !(type_token->inf.isType = this->typeExists(*type_token))
    ) {
        dbg.Error(type_token->rawValue+" is not a valid type!");
        return nullptr;
    }

    //
    Token var_name = this->tokens->eat();

    dbg.Log("var name: "+var_name.rawValue);

    if (this->varExists(var_name)) {
        dbg.Error("Duplicate variable "+var_name.rawValue+" declared on line "+std::to_string(var_name.src.lineNumber));
        return nullptr;
    }

    ast_node_var_decl *res = new ast_node_var_decl();

    res->name = var_name.rawValue;
    res->vty = this->getTypeInfo(*type_token);

    //check for more var stuff
    Token v_next = this->tokens->eat();

    bool hval = false;

    switch (v_next.ty) {
        case tok_semi:
            goto ret;
        case tok_operator: {
            if (v_next.is("="))
                //get variable value
                hval = true;
            else
                goto err;
            break;
        }
        default: {
err:
            dbg.ThrowUnexpectedToken("= or ;", v_next);
            delete res;
            return nullptr;
        }
    }

    if (hval) {

        //just parse binary expression for now
        //but later we can change this for more 
        //funcitonality such as assigning to a
        //funciton or class
        res->asign_val = this->parse_binary_expr();
    }


    //return stuff
ret:
    dbg.Log("Generated Var: "+res->vty.name+" "+res->name+" "+std::to_string(res->asign_val->ty));
    
    this->currentScope->add_var_decl(res->name);

    return res;
}