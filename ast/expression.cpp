#include "expression.h"
#include <stdexcept>
#include <map>
#include <iostream>

namespace Grammar::Expressions {

std::unique_ptr<AST::Expression> Expression::match(parser::Parser& parser, int min_precedence) {
    auto left = match_term(parser);
#ifdef DEBUG
    std::cout << parser.peek_pos() << ", " << parser.peek().get_text() << " in expression matching\n";
#endif

    while (true) {
        const auto token = parser.peek();
        int precedence = token.precedence();
        if (precedence < min_precedence) break;

        parser.next();
        int next_precedence = token.associativity() == lexer::Associativity::LEFT ? precedence + 1 : precedence;
        auto right = Expression::match(parser, next_precedence);

#ifdef DEBUG
        left->print(std::cerr);
        right->print(std::cerr);
        std::cout << token.get_text() << "\n";
#endif

        left = std::make_unique<AST::BinOpExpression>(token, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<AST::Expression> Expression::match_term(parser::Parser& parser) {
#ifdef DEBUG
    std::cout << parser.peek_pos() << ", " << parser.peek().get_text() << " in expression term matching\n";
#endif

    const auto token = parser.peek();
    if (token.is_type(lexer::LPAREN)) {
        parser.next();
        auto expr = Expression::match(parser);
        if (!parser.expect(lexer::RPAREN))
            throw std::runtime_error("expected ')'");
        parser.next();
        return expr;
    }

    if (token.is_type(lexer::NUMBER)) {
        parser.next();
        return std::make_unique<AST::NumberExpression>(std::stoll(token.get_text()));
    }
    
    if (token.is_type(lexer::IDENT)) {
        if (auto eval = Eval::match(parser))
            return eval;
        
        // parser.next() is done in Eval::match
        return std::make_unique<AST::IdentExpression>(token.get_text());
    }
    
    if (token.is_type(lexer::TRUE) || token.is_type(lexer::FALSE)) {
        parser.next();
        return std::make_unique<AST::BoolExpression>(token.get_text());
    }

    if (token.is_type(lexer::NOT)) {
        parser.next();
        auto expr = Expression::match(parser, 70);
        return std::make_unique<AST::UniOpExpression>(token, std::move(expr));
    }

    if (token.is_type(lexer::DASH) || token.is_type(lexer::TILD)) {
        parser.next();
        auto expr = Expression::match(parser, 80);
        return std::make_unique<AST::UniOpExpression>(token, std::move(expr));
    }

    return nullptr;
}

// IDENT(EXPR*)
std::unique_ptr<AST::Expression> Eval::match(parser::Parser& parser) {
    auto name = parser.peek();
    if (!name.is_type(lexer::IDENT))
        return nullptr;
    parser.next();

    if (!parser.expect(lexer::LPAREN))
        return nullptr;
    parser.next();
    
    std::vector<std::unique_ptr<AST::Expression>> params;
    while (!parser.expect(lexer::RPAREN)) {
        auto expr = Expressions::Expression::match(parser);
        if (!expr)
            return nullptr;

        params.push_back(std::move(expr));
        
        if (!parser.expect(lexer::COMMA))
            break;
        parser.next();
    }
    
    if (!parser.expect(lexer::RPAREN))
        return nullptr;
    parser.next();

    return std::make_unique<AST::Eval>(name.get_text(), std::move(params));
}

std::unique_ptr<AST::Type> Type::match(parser::Parser &parser) {
    auto kind = parser.peek();
    parser.next();
    if (kind.is_type(lexer::INT))
        return std::make_unique<AST::Type>(AST::Type::Int());

    if (kind.is_type(lexer::BOOL))
        return std::make_unique<AST::Type>(AST::Type::Bool());

    if (kind.is_type(lexer::VOID))
        return std::make_unique<AST::Type>(AST::Type::Void());

    if (!kind.is_type(lexer::FUNCTION))
        return nullptr;
    
    // function
    if (!parser.expect(lexer::LPAREN))
        return nullptr;
    parser.next();

    std::vector<std::unique_ptr<AST::Type>> param_types;

    while (!parser.expect(lexer::RPAREN)) {
        auto type = Type::match(parser);

        // if (!type || type.is_void()) {
        //     throw std::runtime_error(std::format(
        //         "Error at row {}, col {}! Expected type in procedure '{}' argument declaration!", type.get_row(), type.get_col(), name.get_text()
        //     ));
        // }

        param_types.push_back(std::move(type));

        if (parser.expect(lexer::COMMA))
            parser.next();
    }

    if (!parser.expect(lexer::RPAREN))
        return nullptr;
    parser.next();

    if (!parser.expect(lexer::ARROW))
        return nullptr;
    parser.next();

    // return type can only be first order
    auto return_type = Type::match(parser);
    if (!return_type->is_first_order())
        return nullptr;

    return std::make_unique<AST::Type>(AST::Type::Function(std::move(param_types), std::move(return_type)));
}
    
}; // namespace Grammar::Expressions