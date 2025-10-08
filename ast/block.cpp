#include "block.h"
#include "proc.h"
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

// (VARDECL | PROCDECL)*
std::unique_ptr<AST::Program> Program::match(Parser::Parser& parser) {
    std::vector<std::unique_ptr<AST::Statement>> declarations;
    while (true) {
        // std::cout << "wtf " << parser.peek().get_text() << " " << declarations.size() << "\n";
        if (auto stmt = Statements::VarDecl::match(parser))
            declarations.push_back(std::move(stmt));
        else if (auto stmt = Statements::ProcDecl::match(parser))
            declarations.push_back(std::move(stmt));
        else
            break;
    }

    return std::make_unique<AST::Program>(std::move(declarations));
}

};