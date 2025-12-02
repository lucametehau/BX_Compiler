#include "statement.h"
#include "block.h"
#include "conditional.h"
#include "loop.h"
#include "proc.h"

namespace Grammar::Statements {

// VARDECL | ASSIGN | CALL | IFELSE | WHILE | JUMP | BLOCK | RETURN | LAMBDA
std::unique_ptr<AST::Statement> Statement::match(parser::Parser& parser) {
    auto start_pos = parser.peek_pos();
    if (auto stmt = VarDecl::match(parser))
        return stmt;
    parser.set_pos(start_pos);

    if (auto stmt = Assign::match(parser))
        return stmt;
    parser.set_pos(start_pos);

    if (auto stmt = Call::match(parser))
        return stmt;
    parser.set_pos(start_pos);

    if (auto stmt = IfElse::match(parser))
        return stmt;
    parser.set_pos(start_pos);

    if (auto stmt = While::match(parser))
        return stmt;
    parser.set_pos(start_pos);

    if (auto stmt = Jump::match(parser))
        return stmt;
    parser.set_pos(start_pos);

    if (auto stmt = Block::match(parser))
        return stmt;
    parser.set_pos(start_pos);

    if (auto stmt = Return::match(parser))
        return stmt;
    parser.set_pos(start_pos);

    if (auto stmt = Lambda::match(parser))
        return stmt;
    parser.set_pos(start_pos);

    if (auto stmt = ExpressionStatement::match(parser))
        return stmt;
    parser.set_pos(start_pos);
    
    return nullptr;
}

// var (IDENT = EXPR,)* : TYPE;
std::unique_ptr<AST::Statement> VarDecl::match(parser::Parser& parser) {
    if (!parser.expect(lexer::VAR))
        return nullptr;
    parser.next();

    std::vector<std::pair<std::string, std::unique_ptr<AST::Expression>>> var_inits;
    while (true) {
        auto name = parser.peek();
        if (!name.is_type(lexer::IDENT))
            return nullptr;
        parser.next();

        if (!parser.expect(lexer::EQ))
            return nullptr;
        parser.next();

        auto expr = Expressions::Expression::match(parser);
        if (!expr)
            return nullptr;
        
        var_inits.push_back(std::make_pair(name.get_text(), std::move(expr)));
        
        if (!parser.expect(lexer::COMMA))
            break;
        parser.next();
    }

    if (!parser.expect(lexer::COLON))
        return nullptr;
    parser.next();

    auto type = parser.peek();
    if (!type.is_type(lexer::INT) && !type.is_type(lexer::BOOL))
        return nullptr;
    parser.next();

    if (!parser.expect(lexer::SEMICOLON))
        return nullptr;
    parser.next();

    return std::make_unique<AST::VarDecl>(std::move(var_inits), type);
}

// IDENT = EXPR;
std::unique_ptr<AST::Statement> Assign::match(parser::Parser& parser) {
    auto name = parser.peek();
    if (!name.is_type(lexer::IDENT))
        return nullptr;
    parser.next();

    if (!parser.expect(lexer::EQ))
        return nullptr;
    parser.next();

    auto expr = Expressions::Expression::match(parser);
    if (!expr)
        return nullptr;
    
    if (!parser.expect(lexer::SEMICOLON))
        return nullptr;
    parser.next();

    return std::make_unique<AST::Assign>(name.get_text(), std::move(expr));
}

// EXPR;
std::unique_ptr<AST::Statement> ExpressionStatement::match(parser::Parser& parser) {
    auto expr = Expressions::Expression::match(parser);

    if (!expr)
        return nullptr;

    if (!parser.expect(lexer::SEMICOLON))
        return nullptr;
    parser.next();

    return std::make_unique<AST::ExpressionStatement>(std::move(expr));
}

};