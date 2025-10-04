#include "proc.h"
#include "block.h"

namespace Grammar::Statements {

// return (EXPR)?;
std::unique_ptr<AST::Statement> Return::match(Parser::Parser& parser) {
    if (!parser.expect(Lexer::RETURN))
        return nullptr;
    parser.next();

    auto expr = Expressions::Expression::match(parser);
    if (!expr)
        return nullptr;

    return std::make_unique<AST::Return>(std::move(expr));
}

// def IDENT(PARAM*) (: IDENT)? BLOCK
std::unique_ptr<AST::Statement> ProcDecl::match(Parser::Parser& parser) {
    if (!parser.expect(Lexer::DEF))
        return nullptr;
    parser.next();

    auto name = parser.peek();
    if (!name.is_type(Lexer::IDENT))
        return nullptr;
    parser.next();

    if (!parser.expect(Lexer::LPAREN))
        return nullptr;
    parser.next();

    std::vector<std::unique_ptr<AST::Statement>> params;

    while (!parser.expect(Lexer::RPAREN)) {
        std::vector<std::string> names;
        while (true) {
            auto token = parser.peek();
            if (!token.is_type(Lexer::IDENT))
                return nullptr;
            names.push_back(token.get_text());
            parser.next();

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

        for (auto &name : names)
            params.push_back(std::make_unique<AST::Param>(name, type));
    }

    if (!parser.expect(Lexer::RPAREN))
        return nullptr;
    parser.next();

    // ugly, will repair later
    auto return_type = Lexer::Token(Lexer::VOID, "void", parser.peek().get_row(), parser.peek().get_col());
    if (parser.expect(Lexer::COLON)) {
        parser.next();

        return_type = parser.peek();
        if (!return_type.is_type(Lexer::INT) && !return_type.is_type(Lexer::BOOL))
            return nullptr;
        parser.next();
    }

    auto block = Block::match(parser);
    if (!block)
        return nullptr;

    return std::make_unique<AST::ProcDecl>(name.get_text(), return_type, std::move(params), std::move(block));
}

};