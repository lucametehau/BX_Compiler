#pragma once
#include <string>
#include <map>
#include <fstream>
#include <cassert>

namespace Lexer {

enum Type {
    IDENT, NUMBER, 
    DEF, VAR, INT, PRINT,
    LPAREN, RPAREN, LBRACE, RBRACE, COLON, SEMICOLON,
    AMP, DASH, EQ, HAT, PCENT, PIPE, PLUS, SLASH, 
    STAR, TILD, LTLT, GTGT,
    END
};


enum class Associativity {
    LEFT, RIGHT, NONE
};

struct Operator {
    int precedence;
    Associativity assoc;
};

inline const std::map<Type, Operator> operators = {
    {Type::PIPE, {10, Associativity::LEFT}},
    {Type::HAT, {20, Associativity::LEFT}},
    {Type::AMP, {30, Associativity::LEFT}},
    {Type::LTLT, {40, Associativity::LEFT}},
    {Type::GTGT, {40, Associativity::LEFT}},
    {Type::PLUS, {50, Associativity::LEFT}},
    {Type::DASH, {50, Associativity::LEFT}},
    {Type::STAR, {60, Associativity::LEFT}},
    {Type::SLASH, {60, Associativity::LEFT}},
    {Type::PCENT, {60, Associativity::LEFT}},
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

    [[nodiscard]] int precedence() const {
        const auto it = operators.find(type);
        return it == operators.end() ? -1 : it->second.precedence; // force quit if non operator
    }

    [[nodiscard]] Associativity associativity() const {
        const auto it = operators.find(type);
        return it == operators.end() ? Associativity::NONE : it->second.assoc; // force quit if non operator
    }

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
    {"def", DEF}, {"var", VAR}, {"int", INT}, {"print", PRINT},
    {"<<", LTLT}, {">>", GTGT}
};

inline const std::map<Type, std::string> text_of_token = {
    {LPAREN, "("}, {RPAREN, ")"}, {LBRACE, "{"}, {RBRACE, "}"},
    {COLON, ":"}, {SEMICOLON, ";"}, {AMP, "&"}, {DASH, "-"},
    {EQ, "="}, {HAT, "^"}, {PCENT, "%"}, {PIPE, "|"},
    {PLUS, "+"}, {SLASH, "/"}, {STAR, "*"}, {TILD, "~"},
    {DEF, "def"}, {VAR, "var"}, {INT, "int"}, {PRINT, "print"},
    {LTLT, "<<"}, {GTGT, ">>"}
};

inline const std::map<std::string, std::string> op_code = {
    {"&", "and"}, {"-", "sub"}, {"+", "add"}, {"*", "mul"},
    {"/", "div"}, {"^", "xor"}, {"%", "mod"}, {"|", "or"},
    {"~", "neg"}, {"<<", "shl"}, {">>", "shr"}
};

}; // namespace Lexer