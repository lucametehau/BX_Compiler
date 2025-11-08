#include "conditional.h"
#include "expression.h"
#include "block.h"

namespace Grammar::Statements {

// if (EXPR) BLOCK IFREST
std::unique_ptr<AST::Statement> IfElse::match(parser::Parser& parser) {
    if (!parser.expect(lexer::IF))
        return nullptr;
    parser.next();

    if (!parser.expect(lexer::LPAREN))
        return nullptr;
    parser.next();

    auto expr = Expressions::Expression::match(parser);
    if (!expr)
        return nullptr;

    
    if (!parser.expect(lexer::RPAREN))
        return nullptr;
    parser.next();

    auto then_branch = Statements::Block::match(parser);
    if (!then_branch)
        return nullptr;
    
    auto else_branch = IfRest::match(parser);
    if (else_branch.has_value())
        return std::make_unique<AST::IfElse>(std::move(expr), std::move(then_branch), std::move(else_branch.value()));
    return std::make_unique<AST::IfElse>(std::move(expr), std::move(then_branch));
}

// else IFELSE | BLOCK
std::optional<std::unique_ptr<AST::Statement>> IfRest::match(parser::Parser& parser) {
    if (!parser.expect(lexer::ELSE))
        return std::nullopt;
    parser.next();

    if (auto ifelse = IfElse::match(parser)) {
        return ifelse;
    }

    if (auto block = Statements::Block::match(parser)) {
        return block;
    }

    return std::nullopt;
}

};