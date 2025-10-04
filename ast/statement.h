#pragma once
#include "expression.h"

namespace Grammar::Statements {

struct VarInit {
    static std::unique_ptr<AST::Statement> match(Parser::Parser& parser);
};

struct VarDecl {
    static std::unique_ptr<AST::Statement> match(Parser::Parser& parser);
};

struct Assign {
    static std::unique_ptr<AST::Statement> match(Parser::Parser& parser);
};

struct Print {
    static std::unique_ptr<AST::Statement> match(Parser::Parser& parser);
};

struct Statement {
    static std::unique_ptr<AST::Statement> match(Parser::Parser& parser);
};

struct ProcDecl {
    static std::unique_ptr<AST::Statement> match(Parser::Parser& parser);
};

};