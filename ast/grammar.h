#pragma once
#include "../lexer/token.h"
#include "../parser/parser.h"
#include "ast.h"

namespace Grammar {

template<Lexer::Type T>
class Token {
public:
    static std::unique_ptr<AST::AST> match(Parser::Parser& parser) {
        std::cout << parser.peek_pos() << " xdd in Token\n";
        if (parser.peek().is_type(T)) {
            auto node = std::make_unique<AST::Token>(parser.peek());
            parser.next();
            return node;
        }
        else {
            std::cerr << "token unmatched, needed " << T << "\n";
        }
        return nullptr;
    }
};

template<typename... Rules>
struct Seq {
    static std::unique_ptr<AST::AST> match(Parser::Parser& parser) {
        const auto start_pos = parser.peek_pos();
        std::vector<std::unique_ptr<AST::AST>> rules;

        std::cout << start_pos << " xdd in SEQ\n";

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

        std::cout << matches << " for " << start_pos << " xdd in SEQ\n";

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

        std::cout << start_pos << " xdd in OR\n";

        bool matches = false;
        (([&]() {
            if (matches) return;
            auto node = Rules::match(parser);
            if (node) {
                rule = std::move(node);
                matches = true;
            }
        }()), ...);

        std::cout << matches << " for " << start_pos << " xdd in OR\n";

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

        std::cout << parser.peek_pos() << " xdd in STAR\n";

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