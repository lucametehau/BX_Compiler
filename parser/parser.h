#pragma once
#include "../lexer/token.h"
#include "../ast/ast.h"
#include <vector>
#include <cassert>
#include <memory>

namespace Parser {

class Parser {
private:
    std::vector<Lexer::Token> tokens;
    std::size_t pos;
public:
    Parser() = default;
    Parser(std::vector<Lexer::Token>& tokens) : tokens(std::move(tokens)), pos(0) {}

public:
    [[nodiscard]] constexpr Lexer::Token peek(std::size_t off = 0) const {
        assert(pos + off < tokens.size());
        return tokens[pos + off];
    }

    [[nodiscard]] constexpr std::size_t peek_pos(std::size_t off = 0) const {
        assert(pos + off < tokens.size());
        return pos + off;
    }

    void set_pos(std::size_t _pos) {
        pos = _pos;
    }

    void next() {
        pos++;
    }

    // [[nodiscard]] constexpr std::vector<std::unique_ptr<AST::AST>> parse() const;
};

}; // namespace Parser