#pragma once
#include "../lexer/token.h"
#include "../parser/parser.h"
#include "ast.h"

namespace Grammar {

template<Lexer::Type T>
class Token {
public:
    static std::unique_ptr<AST::AST> match(Parser::Parser& parser) {
#ifdef DEBUG
        std::cout << parser.peek_pos() << " xdd in Token\n";
#endif
        if (parser.peek().is_type(T)) {
            auto node = std::make_unique<AST::Token>(parser.peek());
            parser.next();
            return node;
        }
#ifdef DEBUG
        else {
            std::cerr << "token unmatched, needed " << T << "\n";
        }
#endif
        return nullptr;
    }
};

template<typename... Rules>
struct Seq {
    static std::unique_ptr<AST::AST> match(Parser::Parser& parser) {
        const auto start_pos = parser.peek_pos();
        std::vector<std::unique_ptr<AST::AST>> rules;

#ifdef DEBUG
        std::cout << start_pos << " xdd in SEQ\n";
#endif

        bool matches = true;
        (([&] {
            if (!matches) return;
            auto node = Rules::match(parser);
            if (node) {
                rules.push_back(std::move(node));
            } else {
                matches = false;
            }
        }()), ...);

#ifdef DEBUG
        std::cout << matches << " for " << start_pos << " xdd in SEQ\n";
#endif

        if (matches)
            return std::make_unique<AST::Seq>(std::move(rules));
        else {
            parser.set_pos(start_pos);
            return nullptr;
        }
    }
};

template<typename... Rules>
struct Or {
    static std::unique_ptr<AST::AST> match(Parser::Parser& parser) {
        const auto start_pos = parser.peek_pos();
        std::unique_ptr<AST::AST> rule;

#ifdef DEBUG
        std::cout << start_pos << " xdd in OR\n";
#endif

        bool matches = false;
        (([&]() {
            if (matches) return;
            auto node = Rules::match(parser);
            if (node) {
                rule = std::move(node);
                matches = true;
            }
        }()), ...);

#ifdef DEBUG
        std::cout << matches << " for " << start_pos << " xdd in OR\n";
#endif

        if (matches)
            return std::make_unique<AST::Or>(std::move(rule));
        else {
            parser.set_pos(start_pos);
            return nullptr;
        }
    }
};


template<typename Rule>
struct Star {
    static std::unique_ptr<AST::AST> match(Parser::Parser& parser) {
        std::vector<std::unique_ptr<AST::AST>> rules;

#ifdef DEBUG
        std::cout << parser.peek_pos() << " xdd in STAR\n";
#endif

        while (true) {
            const auto start_pos = parser.peek_pos();
            auto node = Rule::match(parser);

            if (node)
                rules.push_back(std::move(node));
            else {
                parser.set_pos(start_pos);
                break;
            }
        }

        return std::make_unique<AST::Star>(std::move(rules));
    }
};

}; // namespace Grammar