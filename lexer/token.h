#pragma once
#include <string>
#include <map>
#include <fstream>
#include <cassert>
#include <set>

namespace Lexer {

enum Type {
    IDENT, NUMBER, 
    DEF, VAR, INT, BOOL, PRINT, IF, ELSE,
    LPAREN, RPAREN, LBRACE, RBRACE, COLON, SEMICOLON,
    DASH, EQ, PCENT, PLUS, SLASH, STAR,
    TILD, AMP, LTLT, GTGT, HAT, PIPE,
    EQEQ, NEQ, LT, LTE, GT, GTE, ANDAND, OROR, NOT,
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
    {Type::OROR, {3, Associativity::LEFT}},
    {Type::ANDAND, {6, Associativity::LEFT}},
    {Type::PIPE, {10, Associativity::LEFT}},
    {Type::HAT, {20, Associativity::LEFT}},
    {Type::AMP, {30, Associativity::LEFT}},
    {Type::EQEQ, {33, Associativity::NONE}},
    {Type::NEQ, {33, Associativity::NONE}},
    {Type::LT, {36, Associativity::LEFT}},
    {Type::LTE, {36, Associativity::LEFT}},
    {Type::GT, {36, Associativity::LEFT}},
    {Type::GTE, {36, Associativity::LEFT}},
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

    [[nodiscard]] Type get_type() const { return type; }

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
    {"true", BOOL}, {"false", BOOL}, {"if", IF}, {"else", ELSE},
    {"<<", LTLT}, {">>", GTGT}, {"==", EQEQ}, {"!=", NEQ},
    {"<", LT}, {"<=", LTE}, {">", GT}, {">=", GTE},
    {"&&", ANDAND}, {"||", OROR}, {"!", NOT},
};

inline const std::map<Type, std::string> op_code = {
    {AMP, "and"}, {DASH, "sub"}, {PLUS, "add"}, {STAR, "mul"},
    {SLASH, "div"}, {HAT, "xor"}, {PCENT, "mod"}, {PIPE, "or"},
    {TILD, "neg"}, {LTLT, "shl"}, {GTGT, "shr"}
};

inline const std::map<Type, std::string> jump_code = {
    {EQEQ, "jz"}, {NEQ, "jnz"}, {LT, "jl"}, {LTE, "jle"}, 
    {GT, "jg"}, {GTE, "jge"}
};

// binary operators
inline const std::set<Type> bool_binary_operators = {
    ANDAND, OROR, EQEQ, NEQ, LT, LTE, GT, GTE
};

// binary operators
inline const std::set<Type> int_operators = {
    PLUS, DASH, STAR, SLASH, PCENT, GTGT, LTLT, AMP, PIPE, HAT
};

}; // namespace Lexer