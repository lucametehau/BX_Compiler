#include "expression.h"

namespace Grammar::Statements {

using VarDecl = Seq<
    Token<Lexer::VAR>,
    Expressions::Identifier,
    Token<Lexer::EQ>,
    Expressions::Expression,
    Token<Lexer::COLON>,
    Token<Lexer::INT>,
    Token<Lexer::SEMICOLON>
>;

using Assign = Seq<
    Expressions::Identifier,
    Token<Lexer::EQ>,
    Expressions::Expression,
    Token<Lexer::SEMICOLON>
>;

using Print = Seq<
    Token<Lexer::PRINT>,
    Token<Lexer::LPAREN>,
    Expressions::Expression,
    Token<Lexer::RPAREN>,
    Token<Lexer::SEMICOLON>
>;

using Statement = Or<
    VarDecl,
    Assign,
    Print
>;

using Block = Seq<
    Token<Lexer::LBRACE>,
    Star<Statement>,
    Token<Lexer::RBRACE>
>;

using Program = Seq<
    Token<Lexer::DEF>,
    Expressions::Identifier,
    Token<Lexer::LPAREN>,
    Token<Lexer::RPAREN>,
    Block
>;

};