#include "block.h"
#include <iostream>

namespace Grammar::Blocks {

std::unique_ptr<AST::Block> Block::match(Parser::Parser& parser) {
#ifdef DEBUG
    std::cout << parser.peek_pos() << ", " << parser.peek().get_text() << " in block matching\n";
#endif
    if (!parser.peek().is_type(Lexer::LBRACE))
        return nullptr;
    parser.next();

    std::vector<std::unique_ptr<AST::Statement>> statements;
    while (auto stmt = Statements::Statement::match(parser)) {
        statements.push_back(std::move(stmt));
    }

    if (!parser.peek().is_type(Lexer::RBRACE))
        return nullptr;
    parser.next();

    return std::make_unique<AST::Block>(std::move(statements));
}

std::unique_ptr<AST::Program> Program::match(Parser::Parser& parser) {
    if (!parser.peek().is_type(Lexer::DEF))
        return nullptr;
    parser.next();

    if (!parser.peek().is_type(Lexer::IDENT) || parser.peek().get_text() != "main")
        return nullptr;
    parser.next();

    if (!parser.peek().is_type(Lexer::LPAREN))
        return nullptr;
    parser.next();

    if (!parser.peek().is_type(Lexer::RPAREN))
        return nullptr;
    parser.next();

    auto block = Block::match(parser);
    if (!block)
        return nullptr;

    return std::make_unique<AST::Program>(std::move(block));
}

};