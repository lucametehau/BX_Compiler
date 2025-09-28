#pragma once
#include "statement.h"

namespace Grammar::Statements {

struct Block {
    static std::unique_ptr<AST::Block> match(Parser::Parser& parser);
};

struct Program {
    static std::unique_ptr<AST::Program> match(Parser::Parser& parser);
};

};