#include "statement.h"

namespace Grammar::Statements {

std::unique_ptr<AST::Statement> Statement::match(Parser::Parser& parser) {
    if (auto stmt = VarDecl::match(parser))
        return stmt;
    if (auto stmt = Assign::match(parser))
        return stmt;
    if (auto stmt = Print::match(parser))
        return stmt;

    return nullptr;
}

std::unique_ptr<AST::Statement> VarDecl::match(Parser::Parser& parser) {
    if (!parser.expect(Lexer::VAR))
        return nullptr;
    parser.next();

    auto token = parser.peek();
    if (!token.is_type(Lexer::IDENT))
        return nullptr;
    parser.next();

    if (!parser.expect(Lexer::EQ))
        return nullptr;
    parser.next();

    auto expr = Expressions::Expression::match(parser);
    if (!expr)
        return nullptr;

    if (!parser.expect(Lexer::COLON))
        return nullptr;
    parser.next();

    if (!parser.expect(Lexer::INT))
        return nullptr;
    parser.next();

    if (!parser.expect(Lexer::SEMICOLON))
        return nullptr;
    parser.next();

    return std::make_unique<AST::VarDecl>(token.get_text(), std::move(expr));
}

std::unique_ptr<AST::Statement> Assign::match(Parser::Parser& parser) {
    auto token = parser.peek();
    if (!token.is_type(Lexer::IDENT))
        return nullptr;
    parser.next();

    if (!parser.expect(Lexer::EQ))
        return nullptr;
    parser.next();

    auto expr = Expressions::Expression::match(parser);
    if (!expr)
        return nullptr;
    
    if (!parser.expect(Lexer::SEMICOLON))
        return nullptr;
    parser.next();

    return std::make_unique<AST::Assign>(token.get_text(), std::move(expr));
}

std::unique_ptr<AST::Statement> Print::match(Parser::Parser& parser) {
    if (!parser.expect(Lexer::PRINT))
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
    
    if (!parser.expect(Lexer::SEMICOLON))
        return nullptr;
    parser.next();

    return std::make_unique<AST::Print>(std::move(expr));
}

};