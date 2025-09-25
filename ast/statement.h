#include "expression.h"

namespace Grammar::Statements {

struct VarDecl : Seq<
    Token<Lexer::VAR>,
    Expressions::Identifier,
    Token<Lexer::EQ>,
    Expressions::Expression,
    Token<Lexer::COLON>,
    Token<Lexer::INT>,
    Token<Lexer::SEMICOLON>
> {};

struct Assign : Seq<
    Expressions::Identifier,
    Token<Lexer::EQ>,
    Expressions::Expression,
    Token<Lexer::SEMICOLON>
> {};

struct Print : Seq<
    Token<Lexer::PRINT>,
    Token<Lexer::LPAREN>,
    Expressions::Expression,
    Token<Lexer::RPAREN>,
    Token<Lexer::SEMICOLON>
> {};

struct Statement : Or<
    VarDecl,
    Assign,
    Print
> {};

struct Block : Seq<
    Token<Lexer::LBRACE>,
    Star<Statement>,
    Token<Lexer::RBRACE>
> {};

struct Program : Seq<
    Token<Lexer::DEF>,
    Expressions::Identifier,
    Token<Lexer::LPAREN>,
    Token<Lexer::RPAREN>,
    Block
> {};

};