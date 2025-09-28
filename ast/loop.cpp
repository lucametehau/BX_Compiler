#include "loop.h"
#include "block.h"

namespace Grammar::Statements {

std::unique_ptr<AST::Statement> While::match(Parser::Parser& parser) {
    if (!parser.expect(Lexer::WHILE))
        return nullptr;
    parser.next();

    if (!parser.expect(Lexer::LPAREN))
        return nullptr;
    parser.next();
    
    auto expr = Expressions::Expression::match(parser);
    if (!expr)
        return nullptr;

    if (!parser.expect(Lexer::RPAREN))
        return nullptr;
    parser.next();
    
    auto block = Statements::Block::match(parser);
    if (!block)
        return nullptr;

    return std::make_unique<AST::While>(std::move(expr), std::move(block));
}

std::unique_ptr<AST::Statement> Jump::match(Parser::Parser& parser) {
    auto token = parser.peek();

    if (!token.is_type(Lexer::BREAK) && !token.is_type(Lexer::CONTINUE))
        return nullptr;
    parser.next();

    if (!parser.expect(Lexer::SEMICOLON))
        return nullptr;
    parser.next();
    
    return std::make_unique<AST::Jump>(token);
}

};