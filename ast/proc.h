#pragma once
#include "statement.h"

namespace Grammar::Statements {

struct Return {
    static std::unique_ptr<AST::Statement> match(Parser::Parser& parser);
};

struct ProcDecl {
    static std::unique_ptr<AST::Statement> match(Parser::Parser& parser);
};

struct Call {
    static std::unique_ptr<AST::Statement> match(Parser::Parser& parser);
};

};