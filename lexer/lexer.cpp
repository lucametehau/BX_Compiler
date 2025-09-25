#include "lexer.h"
#include <iostream>

namespace Lexer {

void Lexer::skip_ws() {
    while (pos < src.size() && whitespaces.find(src[pos]) != std::string::npos)
        pos++;
}

[[nodiscard]] Token Lexer::next() {
    skip_ws();
    if (pos >= src.size())
        return Token{Type::END, ""};

    // number
    if ((pos + 1 < src.size() && src[pos] == '-' && isdigit(src[pos + 1])) || isdigit(src[pos])) {
        const auto start_pos = pos;
        if (src[pos] == '-')
            pos++;
        while (pos < src.size() && isdigit(src[pos])) 
            pos++;
        return Token{Type::NUMBER, src.substr(start_pos, pos - start_pos)};
    }

    // we have a lexing token
    std::string token_s;
    for (std::size_t len = 1; len <= 5 && pos + len - 1 < src.size(); len++) {
        token_s += src[pos + len - 1];
        if (const auto it = lexing_tokens.find(token_s); it != lexing_tokens.end()) {
            pos += len;
            return Token{it->second, token_s};
        }
    }

    // identifier
    const auto start_pos = pos;
    while (pos < src.size() && (isalpha(src[pos]) || isdigit(src[pos]) || src[pos] == '_'))
        pos++;

    return Token{Type::IDENT, src.substr(start_pos, pos - start_pos)};
}

[[nodiscard]] std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    for (auto token = next(); !token.is_end(); token = next()) {
        tokens.push_back(token);
    }
    return tokens;
}

}; // namespace Lexer