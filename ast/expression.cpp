#include "expression.h"
#include <stdexcept>
#include <map>
#include <iostream>

namespace Grammar::Expressions {

std::unique_ptr<AST::Expression> Expression::match(Parser::Parser& parser, int min_precedence) {
    auto left = match_term(parser);
#ifdef DEBUG
    std::cout << parser.peek_pos() << ", " << parser.peek().get_text() << " in expression matching\n";
#endif

    while (true) {
        const auto token = parser.peek();
        int precedence = token.precedence();
        if (precedence < min_precedence) break;

        parser.next();
        int next_precedence = token.associativity() == Lexer::Associativity::LEFT ? precedence + 1 : precedence;
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

std::unique_ptr<AST::Expression> Expression::match_term(Parser::Parser& parser) {
#ifdef DEBUG
    std::cout << parser.peek_pos() << ", " << parser.peek().get_text() << " in expression term matching\n";
#endif

    const auto token = parser.peek();
    if (token.is_type(Lexer::LPAREN)) {
        parser.next();
        auto expr = Expression::match(parser);
        if (!parser.expect(Lexer::RPAREN))
            throw std::runtime_error("expected ')'");
        parser.next();
        return expr;
    }

    if (token.is_type(Lexer::NUMBER)) {
        parser.next();
        return std::make_unique<AST::NumberExpression>(std::stoll(token.get_text()));
    }
    
    if (token.is_type(Lexer::IDENT)) {
        if (auto eval = Eval::match(parser))
            return eval;
        
        // parser.next() is done in Eval::match
        return std::make_unique<AST::IdentExpression>(token.get_text());
    }
    
    if (token.is_type(Lexer::BOOL)) {
        parser.next();
        return std::make_unique<AST::BoolExpression>(token.get_text());
    }

    if (token.is_type(Lexer::NOT)) {
        parser.next();
        auto expr = Expression::match(parser, 70);
        return std::make_unique<AST::UniOpExpression>(token, std::move(expr));
    }

    if (token.is_type(Lexer::DASH) || token.is_type(Lexer::TILD)) {
        parser.next();
        auto expr = Expression::match(parser, 80);
        return std::make_unique<AST::UniOpExpression>(token, std::move(expr));
    }

    return nullptr;
}

// IDENT(EXPR*)
std::unique_ptr<AST::Expression> Eval::match(Parser::Parser& parser) {
    auto name = parser.peek();
    if (!name.is_type(Lexer::IDENT))
        return nullptr;
    parser.next();

    if (!parser.expect(Lexer::LPAREN))
        return nullptr;
    parser.next();
    
    std::vector<std::unique_ptr<AST::Expression>> params;
    while (true) {
        auto expr = Expressions::Expression::match(parser);
        if (!expr)
            return nullptr;

        params.push_back(std::move(expr));
        
        if (!parser.expect(Lexer::COMMA))
            break;
        parser.next();
    }
    
    if (!parser.expect(Lexer::RPAREN))
        return nullptr;
    parser.next();

    return std::make_unique<AST::Eval>(name.get_text(), std::move(params));
}
    
}; // namespace Grammar::Expressions