#pragma once
#include "logger.hpp"
#include "tokens.hpp"
#define DBG_MODE

enum _ERROR_TYPE {
    err_UnexpectedToken
};

class PgrmDebugger : public Logger {
public:
    PgrmDebugger() : Logger() {

    }
    void ThrowUnexpectedToken(Token got) {
        this->Error("Unexpected token \""+got.rawValue+"\" on line "+std::to_string(got.src.linePos));
    }
    void ThrowUnexpectedToken(std::string expected, Token got) {
        this->ThrowUnexpectedToken(got);
        this->Error("\t Expected: \""+expected+"\"");
        //todo error reasoning
    }
};

static PgrmDebugger dbg = PgrmDebugger();