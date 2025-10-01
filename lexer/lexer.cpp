#include "lexer.h"
#include <iostream>

namespace Lexer {

void Lexer::skip_ws() {
    while (pos < src.size() && whitespaces.find(src[pos]) != std::string::npos) {
        if (src[pos] == '\n')
            row++, col = 1;
        else if (src[pos] == '\t')
            col += 4;
        else
            col++;
        pos++;
    }
}

[[nodiscard]] Token Lexer::next() {
    skip_ws();
    if (pos >= src.size())
        return Token{Type::END, "", row, col};

    const auto start_pos = pos;
    const auto start_col = col;

    // number
    if ((pos + 1 < src.size() && src[pos] == '-' && isdigit(src[pos + 1])) || isdigit(src[pos])) {
        if (src[pos] == '-')
            pos++, col++;
        while (pos < src.size() && isdigit(src[pos])) 
            pos++, col++;
        
        return Token{Type::NUMBER, src.substr(start_pos, pos - start_pos), row, start_col};
    }

    // we have a lexing token
    for (std::size_t len = 10; len >= 1; len--) {
        if (pos + len - 1 >= src.size())
            continue;
        
        auto token_s = src.substr(pos, len);
        if (const auto it = lexing_tokens.find(token_s); it != lexing_tokens.end()) {
            pos += len;
            col += len;
            return Token{it->second, token_s, row, start_col};
        }
    }

    // identifier
    while (pos < src.size() && (isalpha(src[pos]) || isdigit(src[pos]) || src[pos] == '_'))
        pos++, col++;

    return Token{Type::IDENT, src.substr(start_pos, pos - start_pos), row, start_col};
}

[[nodiscard]] std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    for (auto token = next(); !token.is_end(); token = next()) {
        tokens.push_back(token);
    }
    return tokens;
}

}; // namespace Lexer