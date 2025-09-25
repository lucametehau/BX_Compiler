#pragma once
#include <string>
#include <map>
#include <fstream>

namespace Lexer {

enum Type {
    IDENT, NUMBER, 
    DEF, VAR, INT, PRINT,
    LPAREN, RPAREN, LBRACE, RBRACE, COLON, SEMICOLON,
    AMP, DASH, EQ, HAT, PCENT, PIPE, PLUS, SLASH, STAR, TILD,
    END
};

class Token {
private:
    Type type;
    std::string text;

public:
    Token() = default;
    Token(Type type, const char* text) : type(type), text(text) {}
    Token(Type type, const std::string text) : type(type), text(text) {}

    [[nodiscard]] constexpr bool is_type(Type _type) const { return type == _type; }

    [[nodiscard]] constexpr bool is_end() const { return type == Type::END; }

    [[nodiscard]] const std::string get_text() const { return text; }

    friend std::ostream& operator << (std::ostream &os, const Token& token) {
        os << "(" << token.type << ", " << token.text << "), ";
        return os;
    }
};

inline const std::map<std::string, Type> lexing_tokens = {
    {"(", LPAREN}, {")", RPAREN}, {"{", LBRACE}, {"}", RBRACE},
    {":", COLON}, {";", SEMICOLON}, {"&", AMP}, {"-", DASH},
    {"=", EQ}, {"^", HAT}, {"%", PCENT}, {"|", PIPE},
    {"+", PLUS}, {"/", SLASH}, {"*", STAR}, {"~", TILD},
    {"def", DEF}, {"var", VAR}, {"int", INT}, {"print", PRINT}
};

inline const std::map<Type, std::string> text_of_token = {
    {LPAREN, "("}, {RPAREN, ")"}, {LBRACE, "{"}, {RBRACE, "}"},
    {COLON, ":"}, {SEMICOLON, ";"}, {AMP, "&"}, {DASH, "-"},
    {EQ, "="}, {HAT, "^"}, {PCENT, "%"}, {PIPE, "|"},
    {PLUS, "+"}, {SLASH, "/"}, {STAR, "*"}, {TILD, "~"},
    {DEF, "def"}, {VAR, "var"}, {INT, "int"}, {PRINT, "print"}
};

}; // namespace Lexer