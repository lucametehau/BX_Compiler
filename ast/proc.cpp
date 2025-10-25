#include "proc.h"
#include "block.h"

namespace Grammar::Statements {

// return (EXPR)?;
std::unique_ptr<AST::Statement> Return::match(Parser::Parser& parser) {
    if (!parser.expect(Lexer::RETURN))
        return nullptr;
    parser.next();

    auto expr = Expressions::Expression::match(parser);

    if (!parser.expect(Lexer::SEMICOLON))
        return nullptr;
    parser.next();
    return !expr ? std::make_unique<AST::Return>() : std::make_unique<AST::Return>(std::move(expr));
}

// EVAL;
std::unique_ptr<AST::Statement> Call::match(Parser::Parser& parser) {
    auto eval = Expressions::Eval::match(parser);
    if (!eval)
        return nullptr;

    if (!parser.expect(Lexer::SEMICOLON))
        return nullptr;
    parser.next();

    return std::make_unique<AST::Call>(std::move(eval));
}

};