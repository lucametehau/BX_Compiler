#pragma once
#include "expression.h"

namespace Grammar::Statements {

struct VarDecl {
    static std::unique_ptr<AST::Statement> match(parser::Parser& parser);
};

struct Assign {
    static std::unique_ptr<AST::Statement> match(parser::Parser& parser);
};

struct Statement {
    static std::unique_ptr<AST::Statement> match(parser::Parser& parser);
};

};