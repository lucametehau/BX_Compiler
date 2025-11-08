#include "declarations.h"
#include "block.h"

namespace Grammar::Declarations {

// var (IDENT = EXPR,)* : TYPE;
std::unique_ptr<AST::Declaration> GlobalVarDecl::match(parser::Parser& parser) {
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

        if (!parser.expect(lexer::NUMBER) && !parser.expect(lexer::FALSE) && !parser.expect(lexer::TRUE)) {
            throw std::runtime_error(std::format(
                "Global variable '{}' declaration expects a simple integer or boolean expression!", name.get_text()
            ));
        }

        auto expr = Expressions::Expression::match_term(parser);
        
        var_inits.push_back(std::make_pair(name.get_text(), std::move(expr)));
        
        if (!parser.expect(lexer::COMMA))
            break;
        parser.next();
    }

    if (!parser.expect(lexer::COLON)) {
        throw std::runtime_error(std::format(
            "Error at row {}, col {}! Expected type in global variable declaration!", parser.peek().get_row(), parser.peek().get_col()
        ));
    }
    parser.next();

    auto type = parser.peek();
    if (!type.is_type(lexer::INT) && !type.is_type(lexer::BOOL)) {
        throw std::runtime_error(std::format(
            "Error at row {}, col {}! Expected type, got '{}', in global variable declaration!", parser.peek().get_row(), parser.peek().get_col(), type.get_text()
        ));
    }
    parser.next();

    if (!parser.expect(lexer::SEMICOLON))
        return nullptr;
    parser.next();

    return std::make_unique<AST::GlobalVarDecl>(std::move(var_inits), type);
}

// def IDENT(PARAM*) (: IDENT)? BLOCK
std::unique_ptr<AST::Declaration> ProcDecl::match(parser::Parser& parser) {
    if (!parser.expect(lexer::DEF))
        return nullptr;
    parser.next();

    auto name = parser.peek();
    if (!name.is_type(lexer::IDENT))
        return nullptr;
    parser.next();

    if (!parser.expect(lexer::LPAREN))
        return nullptr;
    parser.next();

#ifdef DEBUG
    std::cout << "Matching proc " << name.get_text() << "\n";
#endif

    std::vector<AST::Param> params;

    while (!parser.expect(lexer::RPAREN)) {
        std::vector<std::string> names;
        while (true) {
            auto token = parser.peek();
            if (!token.is_type(lexer::IDENT))
                return nullptr;
            names.push_back(token.get_text());
            parser.next();

            if (!parser.expect(lexer::COMMA))
                break;
            parser.next();
        }

        if (!parser.expect(lexer::COLON)) {
            throw std::runtime_error(std::format(
                "Error at row {}, col {}! Expected type in procedure '{}' argument declaration!", parser.peek().get_row(), parser.peek().get_col(), name.get_text()
            ));
        }

        parser.next();

        auto type = parser.peek();
        if (!type.is_type(lexer::INT) && !type.is_type(lexer::BOOL)) {
            throw std::runtime_error(std::format(
                "Error at row {}, col {}! Expected type in procedure '{}' argument declaration!", type.get_row(), type.get_col(), name.get_text()
            ));
        }
        parser.next();

        for (auto &name : names)
            params.push_back({name, type});

        if (parser.expect(lexer::COMMA))
            parser.next();
    }

    if (!parser.expect(lexer::RPAREN))
        return nullptr;
    parser.next();

    // ugly, will repair later
    auto return_type = lexer::Token(lexer::VOID, "void", parser.peek().get_row(), parser.peek().get_col());
    if (parser.expect(lexer::COLON)) {
        parser.next();

        return_type = parser.peek();
        if (!return_type.is_type(lexer::INT) && !return_type.is_type(lexer::BOOL))
            return nullptr;
        parser.next();
    }

    auto block = Grammar::Statements::Block::match(parser);
    if (!block)
        return nullptr;

    return std::make_unique<AST::ProcDecl>(name.get_text(), return_type, std::move(params), std::move(block));
}

// (GLOBALVARDECL | PROCDECL)*
std::unique_ptr<AST::Program> Program::match(parser::Parser& parser) {
    std::vector<std::unique_ptr<AST::Declaration>> declarations;
    while (true) {
        // std::cout << "wtf " << parser.peek().get_text() << " " << declarations.size() << "\n";
        if (auto stmt = Declarations::GlobalVarDecl::match(parser))
            declarations.push_back(std::move(stmt));
        else if (auto stmt = Declarations::ProcDecl::match(parser))
            declarations.push_back(std::move(stmt));
        else
            break;
    }

    return std::make_unique<AST::Program>(std::move(declarations));
}

};