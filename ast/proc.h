#pragma once
#include "statement.h"

namespace Grammar::Statements {

struct ProcDecl {
    static std::unique_ptr<AST::Statement> match(Parser::Parser& parser);
};

};