#include "loop.h"
#include "block.h"

namespace Grammar::Statements {

// while (EXPR) BLOCK
std::unique_ptr<AST::Statement> While::match(parser::Parser& parser) {
    if (!parser.expect(lexer::WHILE))
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
    
    auto block = Statements::Block::match(parser);
    if (!block)
        return nullptr;

    return std::make_unique<AST::While>(std::move(expr), std::move(block));
}

// break | continue
std::unique_ptr<AST::Statement> Jump::match(parser::Parser& parser) {
    auto token = parser.peek();

    if (!token.is_type(lexer::BREAK) && !token.is_type(lexer::CONTINUE))
        return nullptr;
    parser.next();

    if (!parser.expect(lexer::SEMICOLON))
        return nullptr;
    parser.next();
    
    return std::make_unique<AST::Jump>(token);
}

};