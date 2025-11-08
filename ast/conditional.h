#pragma once
#include "statement.h"

namespace Grammar::Statements {

struct IfElse {
    static std::unique_ptr<AST::Statement> match(parser::Parser& parser);
};

struct IfRest {
    static std::optional<std::unique_ptr<AST::Statement>> match(parser::Parser& parser);
};

};