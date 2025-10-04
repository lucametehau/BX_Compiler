#include "statement.h"
#include "block.h"
#include "conditional.h"
#include "loop.h"

namespace Grammar::Statements {

// VARDECL | ASSIGN | PRINT | IFELSE | WHILE | JUMP | BLOCK
std::unique_ptr<AST::Statement> Statement::match(Parser::Parser& parser) {
    if (auto stmt = VarDecl::match(parser))
        return stmt;
    if (auto stmt = Assign::match(parser))
        return stmt;
    if (auto stmt = Print::match(parser))
        return stmt;
    if (auto stmt = IfElse::match(parser))
        return stmt;
    if (auto stmt = While::match(parser))
        return stmt;
    if (auto stmt = Jump::match(parser))
        return stmt;
    if (auto stmt = Block::match(parser))
        return stmt;

    return nullptr;
}

// IDENT = EXPR
std::unique_ptr<AST::Statement> VarInit::match(Parser::Parser& parser) {
    auto ident = parser.peek();
    if (!ident.is_type(Lexer::IDENT))
        return nullptr;
    parser.next();

    if (!parser.expect(Lexer::EQ))
        return nullptr;
    parser.next();

    auto expr = Expressions::Expression::match(parser);
    if (!expr)
        return nullptr;
    
    return std::make_unique<AST::VarInit>(ident.get_text(), std::move(expr));
}

// var VAR_INIT* : TYPE;
std::unique_ptr<AST::Statement> VarDecl::match(Parser::Parser& parser) {
    if (!parser.expect(Lexer::VAR))
        return nullptr;
    parser.next();

    std::vector<std::unique_ptr<AST::Statement>> var_inits;
    while (true) {
        auto init = VarInit::match(parser);
        if (!init)
            return nullptr;
        
        var_inits.push_back(std::move(init));
        
        if (!parser.expect(Lexer::COMMA))
            break;
        parser.next();
    }

    if (!parser.expect(Lexer::COLON))
        return nullptr;
    parser.next();

    auto type = parser.peek();
    if (!type.is_type(Lexer::INT) && !type.is_type(Lexer::BOOL))
        return nullptr;
    parser.next();

    if (!parser.expect(Lexer::SEMICOLON))
        return nullptr;
    parser.next();

    return std::make_unique<AST::VarDecl>(std::move(var_inits), type);
}

// VAR_INIT;
std::unique_ptr<AST::Statement> Assign::match(Parser::Parser& parser) {
    auto var_init = VarInit::match(parser);
    if (!var_init)
        return nullptr;
    
    if (!parser.expect(Lexer::SEMICOLON))
        return nullptr;
    parser.next();

    return std::make_unique<AST::Assign>(std::move(var_init));
}

// print(EXPR);
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