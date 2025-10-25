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
    [[nodiscard]] Lexer::Token peek(std::size_t off = 0) const {
        assert(pos + off < tokens.size());
        return tokens[pos + off];
    }

    [[nodiscard]] std::size_t peek_pos(std::size_t off = 0) const {
        assert(pos + off < tokens.size());
        return pos + off;
    }

    [[nodiscard]] bool expect(Lexer::Type T) const {
        return pos < tokens.size() && peek().is_type(T);
    }

    [[nodiscard]] bool finished() const {
        return pos >= tokens.size();
    }

    void set_pos(std::size_t _pos) {
        pos = _pos;
    }

    void next() {
        pos++;
    }
};

}; // namespace Parser