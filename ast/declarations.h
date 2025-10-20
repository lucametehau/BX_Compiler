#pragma once
#include "statement.h"

namespace Grammar::Declarations {

struct GlobalVarDecl {
    static std::unique_ptr<AST::Declaration> match(Parser::Parser& parser);
};

struct ProcDecl {
    static std::unique_ptr<AST::Declaration> match(Parser::Parser& parser);
};

struct Program {
    static std::unique_ptr<AST::Program> match(Parser::Parser& parser);
};

};