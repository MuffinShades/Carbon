#pragma once

//includes for various ast files
#include "ast.hpp"
#include "util.hpp"
#include "dbg_tools.hpp"
#include "ast_free.hpp"
/**
 * 
 * Holds all the generic util functions for
 * ast parsing and includes each ast file will
 * probably use
 * 
 */

static bool TokenCompare(Token a, Token b) {
    if (a.raw_checksum != b.raw_checksum) return false;
    return _strCompare(a.rawValue, b.rawValue, a.rawValue.length());
}

static int getOperatorId(std::string op) {
    int pos = 0;
    const size_t ol = op.length();
    for (std::string _ : operators) {
        if (_strCompare(_,op,ol))
            return pos;
        pos ++;
    }

    return -1;
}