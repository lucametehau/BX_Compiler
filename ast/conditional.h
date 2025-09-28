#pragma once
#include "statement.h"

namespace Grammar::Statements {

struct IfElse {
    static std::unique_ptr<AST::Statement> match(Parser::Parser& parser);
};

struct IfRest {
    static std::optional<std::unique_ptr<AST::Statement>> match(Parser::Parser& parser);
};

};