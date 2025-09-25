#pragma once
#include "../lexer/token.h"
#include <vector>
#include <memory>

namespace AST {

struct AST {
    virtual ~AST() = default;
    virtual void print(std::ostream& os, int spaces = 0) const = 0;
};

inline std::ostream& operator << (std::ostream& os, const std::unique_ptr<AST>& ast) {
    if (ast)
        ast->print(os);
    return os;
} 

struct Token : AST {
    Lexer::Token token;
    Token(Lexer::Token token) : token(token) {}

    void print(std::ostream& os, int spaces = 0) const override {
        os << std::string(2 * spaces, ' ') << "[Token] " << token.get_text() << "\n";
    }
};

struct Seq : AST {
    std::vector<std::unique_ptr<AST>> rules;
    Seq(std::vector<std::unique_ptr<AST>> rules) : rules(std::move(rules)) {}

    void print(std::ostream& os, int spaces = 0) const override {
        os << std::string(2 * spaces, ' ') << "[Seq] with " << rules.size() << " rules\n";
        for (auto &rule : rules) {
            if (rule)
                rule->print(os, spaces + 1);
            else
                os << "huh\n";
        }
    }
};

struct Or : AST {
    std::unique_ptr<AST> rule;
    Or(std::unique_ptr<AST> rule) : rule(std::move(rule)) {}

    void print(std::ostream& os, int spaces = 0) const override {
        os << std::string(2 * spaces, ' ') << "[Or]\n";
        if (rule)
            rule->print(os, spaces + 1);
    }
};

struct Star : AST {
    std::vector<std::unique_ptr<AST>> rules;
    Star(std::vector<std::unique_ptr<AST>> rules) : rules(std::move(rules)) {}

    void print(std::ostream& os, int spaces = 0) const override {
        os << std::string(2 * spaces, ' ') << "[Star]\n";
        for (auto &rule : rules) {
            if (rule)
                rule->print(os, spaces + 1);
        }
    }
};

}; // namespace AST