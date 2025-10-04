#pragma once
#include "../lexer/token.h"
#include "../mm/tac.h"
#include "../mm/mm.h"
#include <vector>
#include <memory>
#include <cstdint>
#include <variant>
#include <optional>

namespace AST {

enum class Type {
    INT,
    BOOL,
    NONE
};

struct AST {
    virtual ~AST() = default;
    
    virtual void print(std::ostream& os, int spaces = 0) = 0;
    
    virtual std::vector<TAC> munch(MM::MM& muncher) = 0;
};

template<typename T, typename = std::enable_if_t<std::is_base_of_v<AST, T>>> // wtf
inline std::ostream& operator << (std::ostream& os,  std::unique_ptr<AST>& ast) {
    if (ast)
        ast->print(os);
    return os;
}

/*
Expressions
*/

struct Expression : AST {
protected:
    Type type = Type::NONE;

public:
    void print(std::ostream& os, int spaces = 0) override = 0;

    [[nodiscard]] Type get_type() { return type; }

    [[nodiscard]] virtual std::vector<TAC> munch(MM::MM& muncher) override = 0;
    [[nodiscard]] virtual std::vector<TAC> munch_bool([[maybe_unused]] MM::MM& muncher, 
                                                      [[maybe_unused]] std::string label_true, 
                                                      [[maybe_unused]] std::string label_false) {
        return {};
    }
};

struct NumberExpression : Expression {
    int64_t value;

    NumberExpression(int64_t value) : value(value) {
        type = Type::INT;
    }

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[NUMBER] " << value << "\n";
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;    
};

struct IdentExpression : Expression {
    std::string name;

    IdentExpression(std::string name) : name(name) {
        type = Type::INT; // only int variables for now
    }

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[IDENT] " << name << "\n";
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;
};

struct BoolExpression : Expression {
    bool value;

    BoolExpression(std::string s_value) {
        value = s_value == "true";
        type = Type::BOOL;
    }

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[BOOL] " << value << "\n";
    }

    [[nodiscard]] std::vector<TAC> munch([[maybe_unused]] MM::MM& muncher) override {
        return {};
    }

    [[nodiscard]] std::vector<TAC> munch_bool([[maybe_unused]] MM::MM& muncher, std::string label_true, std::string label_false) override;
};

struct UniOpExpression : Expression {
    Lexer::Token token;
    std::unique_ptr<Expression> expr;

    UniOpExpression(Lexer::Token token, std::unique_ptr<Expression> _expr) : token(token), expr(std::move(_expr)) {
        type = expr->get_type();
    }

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[UniOp] " << token.get_text() << "\n";
        if (expr)
            expr->print(os, spaces + 1);
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;
    [[nodiscard]] std::vector<TAC> munch_bool(MM::MM& muncher, std::string label_true, std::string label_false) override;
};

struct BinOpExpression : Expression {
    Lexer::Token token;
    std::unique_ptr<Expression> left, right;

    BinOpExpression(Lexer::Token token, std::unique_ptr<Expression> _left, std::unique_ptr<Expression> _right) 
        : token(token), left(std::move(_left)), right(std::move(_right)) {
        type = left->get_type();
    }

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[BinOp] " << token.get_text() << "\n";
        if (left)
            left->print(os, spaces + 1);
        if (right)
            right->print(os, spaces + 1);
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;
    [[nodiscard]] std::vector<TAC> munch_bool(MM::MM& muncher, std::string label_true, std::string label_false) override;
};

/*
Statements
*/
struct Statement : AST {
    void print(std::ostream& os, int spaces = 0) override = 0;

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override = 0;
};

struct VarInit : Statement {
    std::string name;
    std::unique_ptr<Expression> expr;

    VarInit(std::string name, std::unique_ptr<Expression> expr) : name(name), expr(std::move(expr)) {}
    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[VarInit] " << name << " =\n";
        if (expr)
            expr->print(os, spaces + 1);
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override {
        throw "xd\n";
    }
};

struct Param : Statement {
    std::string name;
    Lexer::Token type;

    Param(std::string name, Lexer::Token type) : name(name), type(type) {}

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[Param] " << name << " : " << type.get_text() << "\n";
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override {
        throw "lol\n";
    }
};

struct VarDecl : Statement {
    std::vector<std::unique_ptr<Statement>> var_inits;
    Lexer::Token type;

    VarDecl(std::vector<std::unique_ptr<Statement>> var_inits, Lexer::Token type) : var_inits(std::move(var_inits)), type(std::move(type)) {}
    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[VarDecl] Type " << type.get_text() << "\n";
        for (auto &init : var_inits) {
            if (init)
                init->print(os, spaces + 1);
        }
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;
};

struct Assign : Statement {
    std::unique_ptr<Statement> var_init;

    Assign(std::unique_ptr<Statement> var_init) : var_init(std::move(var_init)) {}
    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[Assign]\n";
        if (var_init)
            var_init->print(os, spaces + 1);
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;
};

struct Print : Statement {
    std::unique_ptr<Expression> expr;

    Print(std::unique_ptr<Expression> expr) : expr(std::move(expr)) {}

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[Print]\n";
        if (expr)
            expr->print(os, spaces + 1);
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;
};

struct Jump : Statement {
    Lexer::Token token;

    Jump(Lexer::Token token) : token(token) {}

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ');
        os << (token.is_type(Lexer::CONTINUE) ? "[Continue]" : "[Break]");
        os << "\n";
    }
};

/*
Block
*/
struct Block : Statement {
    std::vector<std::unique_ptr<Statement>> statements;

    Block() = default; // needed for Program
    Block(std::vector<std::unique_ptr<Statement>> statements) : statements(std::move(statements)) {}

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[Block]\n";
        for (auto &statement : statements) {
            if (statement)
                statement->print(os, spaces + 1);
        }
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;
};

/*
If Else
*/
struct IfElse : Statement {
    std::unique_ptr<Expression> expr;
    std::unique_ptr<Block> then_branch;
    std::optional<std::unique_ptr<Statement>> else_branch;
    
    IfElse() = default;
    IfElse(std::unique_ptr<Expression> expr, std::unique_ptr<Block> then_branch) 
            : expr(std::move(expr)), then_branch(std::move(then_branch)) {}
    IfElse(std::unique_ptr<Expression> expr, std::unique_ptr<Block> then_branch,
           std::unique_ptr<Statement> else_branch) : expr(std::move(expr)), then_branch(std::move(then_branch)), else_branch(std::move(else_branch)) {}

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[If]\n";
        if (expr)
            expr->print(os, spaces + 1);
        os << std::string(2 * spaces, ' ') << "[Then]\n";
        if (then_branch)
            then_branch->print(os, spaces + 1);

        // wtf have i gotten myself into
        if (else_branch.has_value()) {
            os << std::string(2 * spaces, ' ') << "[Else]\n";
            else_branch.value()->print(os, spaces + 1);
        }
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;
};

/*
While
*/
struct While : Statement {
    std::unique_ptr<Expression> expr;
    std::unique_ptr<Block> block;

    While(std::unique_ptr<Expression> expr, std::unique_ptr<Block> block) : expr(std::move(expr)), block(std::move(block)) {}

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[While]\n";
        if (expr)
            expr->print(os, spaces + 1);
        os << std::string(2 * spaces, ' ') << "[Do]\n";
        if (block)
            block->print(os, spaces + 1);
    }
};

/*
Procedures
*/
struct ProcDecl : Statement {
    std::string name;
    Lexer::Token return_type;
    std::vector<std::unique_ptr<Statement>> params;
    std::unique_ptr<Block> block;

    ProcDecl(std::string name, Lexer::Token return_type, std::vector<std::unique_ptr<Statement>> params, std::unique_ptr<Block> block) 
        : name(name), return_type(return_type), params(std::move(params)), block(std::move(block)) {}

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[Procedure] " << name;
        if (return_type.is_type(Lexer::INT) || return_type.is_type(Lexer::BOOL))
            os << " -> " << return_type.get_text();
        os << "\n";
        for (auto &param : params) {
            if (param)
                param->print(os, spaces + 1);
        }
        if (block)
            block->print(os, spaces + 2);
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;
};

/*
Program
*/
struct Program : Block {
    std::vector<std::unique_ptr<Statement>> declarations;

    Program(std::vector<std::unique_ptr<Statement>> declarations) : declarations(std::move(declarations)) {}

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[Program]\n";
        for (auto &declaration : declarations) {
            if (declaration)
                declaration->print(os, spaces + 1);
        }
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;
};

}; // namespace AST