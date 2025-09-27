#include "conditional.h"
#include "expression.h"
#include "block.h"

namespace Grammar::Statements {

std::unique_ptr<AST::Statement> IfElse::match(Parser::Parser& parser) {
    if (!parser.expect(Lexer::IF))
        return nullptr;
    parser.next();

    if (!parser.expect(Lexer::LPAREN))
        return nullptr;
    parser.next();

    auto expr = Expressions::BoolExpression::match(parser);
    if (!expr)
        return nullptr;

    
    if (!parser.expect(Lexer::RPAREN))
        return nullptr;
    parser.next();

    auto then_branch = Blocks::Block::match(parser);
    if (!then_branch)
        return nullptr;
    
    auto else_branch = IfRest::match(parser);
    if (else_branch.has_value())
        return std::make_unique<AST::IfElse>(std::move(expr), std::move(then_branch), std::move(else_branch.value()));
    return std::make_unique<AST::IfElse>(std::move(expr), std::move(then_branch));
}

std::optional<std::variant<std::unique_ptr<AST::IfElse>, std::unique_ptr<AST::Block>>> IfRest::match(Parser::Parser& parser) {
    if (!parser.expect(Lexer::ELSE))
        return std::nullopt;
    parser.next();

    // what is this even????? will have to check later
    if (auto ifelse = IfElse::match(parser)) {
        return std::variant<std::unique_ptr<AST::IfElse>, std::unique_ptr<AST::Block>>(
            std::unique_ptr<AST::IfElse>(static_cast<AST::IfElse*>(ifelse.release()))
        );
    }

    if (auto block = Blocks::Block::match(parser)) {
        return std::variant<std::unique_ptr<AST::IfElse>, std::unique_ptr<AST::Block>>(
            std::unique_ptr<AST::Block>(static_cast<AST::Block*>(block.release()))
        );
    }

    return std::nullopt;
}

};