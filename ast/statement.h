#include "expression.h"

namespace Grammar::Statements {

struct VarDecl {
    static std::unique_ptr<AST::Statement> match(Parser::Parser& parser) {
        if (!parser.peek().is_type(Lexer::VAR))
            return nullptr;
        parser.next();

        auto token = parser.peek();
        if (!token.is_type(Lexer::IDENT))
            return nullptr;
        parser.next();

        if (!parser.peek().is_type(Lexer::EQ))
            return nullptr;
        parser.next();

        auto expr = Expressions::Expression::match(parser);
        if (!expr)
            return nullptr;

        if (!parser.peek().is_type(Lexer::COLON))
            return nullptr;
        parser.next();

        if (!parser.peek().is_type(Lexer::INT))
            return nullptr;
        parser.next();

        if (!parser.peek().is_type(Lexer::SEMICOLON))
            return nullptr;
        parser.next();

        return std::make_unique<AST::VarDecl>(token.get_text(), std::move(expr));
    }
};

struct Assign {
    static std::unique_ptr<AST::Statement> match(Parser::Parser& parser) {
        auto token = parser.peek();
        if (!token.is_type(Lexer::IDENT))
            return nullptr;
        parser.next();

        if (!parser.peek().is_type(Lexer::EQ))
            return nullptr;
        parser.next();

        auto expr = Expressions::Expression::match(parser);
        if (!expr)
            return nullptr;
        
        if (!parser.peek().is_type(Lexer::SEMICOLON))
            return nullptr;
        parser.next();

        return std::make_unique<AST::Assign>(token.get_text(), std::move(expr));
    }
};

struct Print {
    static std::unique_ptr<AST::Statement> match(Parser::Parser& parser) {
        if (!parser.peek().is_type(Lexer::PRINT))
            return nullptr;
        parser.next();

        if (!parser.peek().is_type(Lexer::LPAREN))
            return nullptr;
        parser.next();

        auto expr = Expressions::Expression::match(parser);
        if (!expr)
            return nullptr;
        
        if (!parser.peek().is_type(Lexer::RPAREN))
            return nullptr;
        parser.next();
        
        if (!parser.peek().is_type(Lexer::SEMICOLON))
            return nullptr;
        parser.next();

        return std::make_unique<AST::Print>(std::move(expr));
    }
};

struct Statement {
    static std::unique_ptr<AST::Statement> match(Parser::Parser& parser) {
        if (auto stmt = VarDecl::match(parser))
            return stmt;
        if (auto stmt = Assign::match(parser))
            return stmt;
        if (auto stmt = Print::match(parser))
            return stmt;

        return nullptr;
    }
};

};