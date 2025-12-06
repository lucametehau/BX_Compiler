#include "proc.h"
#include "block.h"
#include "expression.h"

namespace Grammar::Statements {

// return (EXPR)?;
std::unique_ptr<AST::Statement> Return::match(parser::Parser& parser) {
    if (!parser.expect(lexer::RETURN))
        return nullptr;
    parser.next();

    auto expr = Expressions::Expression::match(parser);

    if (!parser.expect(lexer::SEMICOLON))
        return nullptr;
    parser.next();
    return !expr ? std::make_unique<AST::Return>() : std::make_unique<AST::Return>(std::move(expr));
}

// EVAL;
std::unique_ptr<AST::Statement> Call::match(parser::Parser& parser) {
    auto eval = Expressions::Eval::match(parser);
    if (!eval)
        return nullptr;

    if (!parser.expect(lexer::SEMICOLON))
        return nullptr;
    parser.next();

    return std::make_unique<AST::Call>(std::move(eval));
}


// def IDENT(PARAM*) (: IDENT)? BLOCK
std::unique_ptr<AST::Statement> Lambda::match(parser::Parser& parser) {
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
    std::cout << "Matching lambda " << name.get_text() << "\n";
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

        auto type = Expressions::Type::match(parser);

        // if (!type || type->is_void()) {
        //     throw std::runtime_error(std::format(
        //         "Error at row {}, col {}! Expected type in procedure '{}' argument declaration!",
        //         parser.peek().get_row(), parser.peek().get_col(), name.get_text()
        //     ));
        // }
        // parser.next();

        for (auto &name : names)
            params.push_back(AST::Param(name, std::move(type)));

        if (parser.expect(lexer::COMMA))
            parser.next();
    }

    if (!parser.expect(lexer::RPAREN))
        return nullptr;
    parser.next();

    auto return_type = std::make_unique<AST::Type>(AST::Type::Void());

    if (parser.expect(lexer::COLON)) {
        parser.next();
        return_type = Expressions::Type::match(parser);
        if (!return_type || !return_type->is_first_order())
            return nullptr;
    }

    auto block = Grammar::Statements::Block::match(parser);
    if (!block)
        return nullptr;

#ifdef DEBUG
    std::cout << "Matched lambda " << name.get_text() << "\n";
#endif

    return std::make_unique<AST::Lambda>(name.get_text(), std::move(return_type), std::move(params), std::move(block));
}

};