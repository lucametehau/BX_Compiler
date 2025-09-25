#include "grammar.h"

namespace Grammar::Expressions {

struct Expression;

using Number = Token<Lexer::NUMBER>;
using Identifier = Token<Lexer::IDENT>;

using UniOpExpression = Seq<
    Or<
        Token<Lexer::DASH>,
        Token<Lexer::TILD>
    >,
    Expression
>;

using BinOpExpression = Seq<
    Expression,
    Or<
        Token<Lexer::PLUS>,
        Token<Lexer::DASH>,
        Token<Lexer::STAR>,
        Token<Lexer::SLASH>,
        Token<Lexer::PCENT>,
        Token<Lexer::AMP>,
        Token<Lexer::PIPE>,
        Token<Lexer::HAT>
    >,
    Expression
>;

struct Expression : Or<
    Number,
    Identifier,
    Seq<
        Token<Lexer::LPAREN>,
        Expression,
        Token<Lexer::RPAREN>
    >,
    BinOpExpression,
    UniOpExpression
> {};
    
}; // namespace Grammar::Expressions