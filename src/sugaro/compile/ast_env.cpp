#include "ast_util.hpp"
/**
 * 
 * For all the ast enviroment stuff
 * 
 */


/**
 * 
 * varExists
 * 
 * Determines if a variable exists give a token
 * 
 */
bool AstGenerator::varExists(Token t) {
    if (!this->currentScope) return false;
    
    for (auto& tk : this->currentScope->declaredVars)
        if (TokenCompare(t, tk)) return true;

    return false;
}

bool AstGenerator::fnExists(Token t) {
    if (!this->currentScope) return false;
    
    for (auto& tk : this->currentScope->declaredFunctions)
        if (TokenCompare(t, tk)) return true;

    return false;
}

//check if a given type exists
bool AstGenerator::typeExists(Token t) {
    if (!this->currentScope) return false;
    const size_t rvl = t.rawValue.length();
    for (auto& ty : primitiveTypes)
        if (_strCompare(t.rawValue, ty.name, rvl)) return true;
    for (auto& tk : this->currentScope->declaredTypes)
        if (TokenCompare(t, tk.origin)) return true;
    return false;
}

//get ast type info
BasicType AstGenerator::getTypeInfo(Token t) {
    if (!this->currentScope) return {};
    const size_t rvl = t.rawValue.length();

    //primitive types
    size_t p_i = 0;
    for (auto& ty : primitiveTypes) {
        if (_strCompare(t.rawValue, ty.name, rvl)) 
            return primitiveTypes[p_i];
        p_i++;
    }
    
    //declared types
    for (auto& tk : this->currentScope->declaredTypes)
        //assemble / get custom type data
        if (TokenCompare(t, tk.origin)) {
            return tk;
        }

    //shoot type don't exist :/
    dbg.Warn("Could not find type: "+t.rawValue+" | This may trigger an error");
    return {};
}