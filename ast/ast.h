#pragma once
#include "../lexer/token.h"
#include "../mm/tac.h"
#include "../mm/mm.h"
#include <vector>
#include <memory>
#include <cstdint>

namespace AST {

struct AST {
    virtual ~AST() = default;
    
    virtual void print(std::ostream& os, int spaces = 0) const = 0;
    
    virtual std::vector<TAC> munch(MM::MM& muncher) const = 0;
};

template<typename T, typename = std::enable_if_t<std::is_base_of_v<AST, T>>> // wtf
inline std::ostream& operator << (std::ostream& os, const std::unique_ptr<AST>& ast) {
    if (ast)
        ast->print(os);
    return os;
}

/*
Expressions
*/

struct Expression : AST {
    void print(std::ostream& os, int spaces = 0) const override = 0;

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) const override = 0;
};

struct NumberExpression : Expression {
    int64_t value;

    NumberExpression(int64_t value) : value(value) {}

    void print(std::ostream& os, int spaces = 0) const override {
        os << std::string(2 * spaces, ' ') << "[NUMBER] " << value << "\n";
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) const override;
};

struct IdentExpression : Expression {
    std::string name;

    IdentExpression(std::string name) : name(name) {}

    void print(std::ostream& os, int spaces = 0) const override {
        os << std::string(2 * spaces, ' ') << "[IDENT] " << name << "\n";
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) const override;
};

struct UniOpExpression : Expression {
    Lexer::Token token;
    std::unique_ptr<Expression> expr;

    UniOpExpression(Lexer::Token token, std::unique_ptr<Expression> expr) : token(token), expr(std::move(expr)) {}

    void print(std::ostream& os, int spaces = 0) const override {
        os << std::string(2 * spaces, ' ') << "[UniOp] " << token.get_text() << "\n";
        if (expr)
            expr->print(os, spaces + 1);
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) const override;
};

struct BinOpExpression : Expression {
    Lexer::Token token;
    std::unique_ptr<Expression> left, right;

    BinOpExpression(Lexer::Token token, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right) 
        : token(token), left(std::move(left)), right(std::move(right)) {}

    void print(std::ostream& os, int spaces = 0) const override {
        os << std::string(2 * spaces, ' ') << "[BinOp] " << token.get_text() << "\n";
        if (left)
            left->print(os, spaces + 1);
        if (right)
            right->print(os, spaces + 1);
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) const override;
};

/*
Statements
*/
struct Statement : AST {
    void print(std::ostream& os, int spaces = 0) const override = 0;

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) const override = 0;
};

struct VarDecl : Statement {
    std::string name;
    std::unique_ptr<Expression> expr;

    VarDecl(std::string name, std::unique_ptr<Expression> expr) : name(name), expr(std::move(expr)) {}
    void print(std::ostream& os, int spaces = 0) const override {
        os << std::string(2 * spaces, ' ') << "[VarDecl] " << name << " =\n";
        if (expr)
            expr->print(os, spaces + 1);
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) const override;
};

struct Assign : Statement {
    std::string name;
    std::unique_ptr<Expression> expr;

    Assign(std::string name, std::unique_ptr<Expression> expr) : name(name), expr(std::move(expr)) {}
    void print(std::ostream& os, int spaces = 0) const override {
        os << std::string(2 * spaces, ' ') << "[Assign] " << name << " =\n";
        if (expr)
            expr->print(os, spaces + 1);
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) const override;
};

struct Print : Statement {
    std::unique_ptr<Expression> expr;

    Print(std::unique_ptr<Expression> expr) : expr(std::move(expr)) {}

    void print(std::ostream& os, int spaces = 0) const override {
        os << std::string(2 * spaces, ' ') << "[Print]\n";
        if (expr)
            expr->print(os, spaces + 1);
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) const override;
};

/*
Special rules
*/
struct Block : AST {
    std::vector<std::unique_ptr<Statement>> statements;

    Block() = default; // needed for Program
    Block(std::vector<std::unique_ptr<Statement>> statements) : statements(std::move(statements)) {}

    void print(std::ostream& os, int spaces = 0) const override {
        os << std::string(2 * spaces, ' ') << "[Block]\n";
        for (auto &statement : statements) {
            if (statement)
                statement->print(os, spaces + 1);
        }
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) const override;
};

struct Program : Block {
    std::unique_ptr<Block> block;

    Program(std::unique_ptr<Block> block) : block(std::move(block)) {}

    void print(std::ostream& os, int spaces = 0) const override {
        os << std::string(2 * spaces, ' ') << "[Program]\n";
        if (block)
            block->print(os, spaces + 1);
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) const override;
};

}; // namespace AST