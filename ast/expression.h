#pragma once
#include "ast.h"
#include "../parser/parser.h"

namespace Grammar::Expressions {

struct Expression {
    static std::unique_ptr<AST::Expression> match(parser::Parser& parser, int min_precedence = 0);

    static std::unique_ptr<AST::Expression> match_term(parser::Parser& parser);
};

struct Eval {
    static std::unique_ptr<AST::Expression> match(parser::Parser& parser);
};
    
}; // namespace Grammar::Expressions