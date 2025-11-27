#pragma once
#include "statement.h"

namespace Grammar::Statements {

struct Return {
    static std::unique_ptr<AST::Statement> match(parser::Parser& parser);
};

struct Call {
    static std::unique_ptr<AST::Statement> match(parser::Parser& parser);
};

struct Lambda {
    static std::unique_ptr<AST::Statement> match(parser::Parser& parser);
};

};