#include "block.h"
#include "proc.h"
#include "declarations.h"
#include <iostream>

namespace Grammar::Statements {

// { STATEMENT* }
std::unique_ptr<AST::Block> Block::match(Parser::Parser& parser) {
    if (!parser.expect(Lexer::LBRACE))
        return nullptr;
    parser.next();

#ifdef DEBUG
    std::cout << parser.peek_pos() << ", " << parser.peek().get_text() << " in block matching\n";
#endif

    std::vector<std::unique_ptr<AST::Statement>> statements;
    while (auto stmt = Statements::Statement::match(parser)) {
        statements.push_back(std::move(stmt));
    }

    if (!parser.expect(Lexer::RBRACE))
        return nullptr;
    parser.next();

    return std::make_unique<AST::Block>(std::move(statements));
}

};