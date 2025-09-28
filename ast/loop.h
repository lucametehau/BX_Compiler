#pragma once
#include "statement.h"

namespace Grammar::Statements {

struct While {
    static std::unique_ptr<AST::Statement> match(Parser::Parser& parser);
};

struct Jump {
    static std::unique_ptr<AST::Statement> match(Parser::Parser& parser);
};

};