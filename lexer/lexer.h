#pragma once
#include "token.h"
#include <vector>
#include <string>
#include <cassert>

namespace lexer {

class Lexer {
private:
    static constexpr std::string_view whitespaces{" \t\n"};
    std::size_t pos;
    std::size_t row, col;
    std::string src;

public:
    Lexer() = default;
    Lexer(std::string &&src) : pos(0), row(1), col(1), src(std::move(src)) {}

    ~Lexer() = default;

    Lexer(const Lexer& other) = delete;
    Lexer& operator = (const Lexer& other) = delete;

private:
    void skip_ws();

public:
    [[nodiscard]] Token next();

    [[nodiscard]] std::vector<Token> tokenize();
};

}; // namespace lexer