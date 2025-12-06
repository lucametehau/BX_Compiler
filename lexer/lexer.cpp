#include "lexer.h"
#include <iostream>
#include <format>

namespace lexer {

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

    // comment start
    if (pos + 1 < src.size() && src[pos] == src[pos + 1] && src[pos] == '/') {
        pos += 2;
        // skip everything until new line
        while (pos < src.size() && src[pos] != '\n')
            pos++;
        row++, col = 1;

        skip_ws();
    }

    // in tests, there is the extern keyword, which we ignoree
#ifdef TEST
    if (pos + 2 < src.size() && src[pos] == 'e' && src[pos+1] == 'x' && src[pos+2] == 't') {
        // skip everything until new line
        while (pos < src.size() && src[pos] != '\n')
            pos++;
        row++, col = 1;

        skip_ws();
    }
#endif
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

    if (pos == start_pos) {
        throw std::runtime_error(std::format(
            "Unexpected character at row {}, col {}!", row, col
        ));
    }

    return Token{Type::IDENT, src.substr(start_pos, pos - start_pos), row, start_col};
}

[[nodiscard]] std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    for (auto token = next(); !token.is_end(); token = next()) {
        tokens.push_back(token);
    }
    return tokens;
}

}; // namespace lexer