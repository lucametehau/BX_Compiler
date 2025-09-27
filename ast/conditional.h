#pragma once
#include "statement.h"

namespace Grammar::Statements {

struct IfElse {
    static std::unique_ptr<AST::Statement> match(Parser::Parser& parser);
};

struct IfRest {
    static std::optional<std::variant<std::unique_ptr<AST::IfElse>, std::unique_ptr<AST::Block>>> match(Parser::Parser& parser);
};

};