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
    MM::Type type = MM::Type::NONE;

public:
    void print(std::ostream& os, int spaces = 0) override = 0;

    [[nodiscard]] MM::Type get_type() { return type; }

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
        type = MM::Type::INT;
    }

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[NUMBER] " << value << "\n";
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;    
};

struct IdentExpression : Expression {
    std::string name;

    IdentExpression(std::string name) : name(name) {
        // type = MM::Type::INT; // only int variables for now
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
        type = MM::Type::BOOL;
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
Function/Procedure eval
*/

struct Eval : Expression {
    std::string name;
    std::vector<std::unique_ptr<Expression>> params;

    Eval(std::string name, std::vector<std::unique_ptr<Expression>> params) : name(std::move(name)), params(std::move(params)) {}

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[Eval] " << name << "\n";
        for (auto &expr : params) {
            if (expr)
                expr->print(os, spaces + 1);
        }
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;
};

/*
Statements
*/
struct Statement : AST {
    void print(std::ostream& os, int spaces = 0) override = 0;

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override = 0;
};

struct Param {
    std::string name;
    Lexer::Token type;

    Param(std::string name, Lexer::Token type) : name(name), type(type) {}

    void print(std::ostream& os, int spaces = 0) {
        os << std::string(2 * spaces, ' ') << "[Param] " << name << " : " << type.get_text() << "\n";
    }
};

struct VarDecl : Statement {
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> var_inits;
    MM::Type type;

    VarDecl(std::vector<std::pair<std::string, std::unique_ptr<Expression>>> var_inits, Lexer::Token _type) : var_inits(std::move(var_inits)) {
        type = MM::lexer_to_mm_type[_type.get_type()];
    }

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[VarDecl] Type " << MM::type_text[type] << "\n";
        for (auto &[name, expr] : var_inits) {
            os << std::string(2 * spaces + 2, ' ') << name << " = \n";
            if (expr)
                expr->print(os, spaces + 2);
        }
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;
};

struct Assign : Statement {
    std::string name;
    std::unique_ptr<Expression> expr;

    Assign(std::string name, std::unique_ptr<Expression> expr) : name(std::move(name)), expr(std::move(expr)) {}

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[Assign] " << name << " = \n";
        if (expr)
            expr->print(os, spaces + 1);
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;
};

struct Call : Statement {
    std::unique_ptr<Expression> eval;

    Call(std::unique_ptr<Expression> eval) : eval(std::move(eval)) {}

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[Call]\n";
        if (eval)
            eval->print(os, spaces + 1);
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

struct Return : Statement {
    std::unique_ptr<Expression> expr;

    Return() : expr(nullptr) {}
    Return(std::unique_ptr<Expression> expr) : expr(std::move(expr)) {}

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[Return]\n";
        if (expr)
            expr->print(os, spaces + 1);
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;
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
Declarations
*/
struct Declaration : AST {
    virtual void declare(MM::MM& muncher) = 0;

    void print(std::ostream& os, int spaces = 0) override = 0;

    [[nodiscard]] virtual std::vector<TAC> munch(MM::MM& muncher) override = 0;
};

struct GlobalVarDecl : Declaration {
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> var_inits;
    MM::Type type;

    GlobalVarDecl(std::vector<std::pair<std::string, std::unique_ptr<Expression>>> var_inits, Lexer::Token _type) : var_inits(std::move(var_inits)) {
        type = MM::lexer_to_mm_type[_type.get_type()];
    }

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[VarDecl] Type " << MM::type_text[type] << "\n";
        for (auto &[name, expr] : var_inits) {
            os << std::string(2 * spaces + 2, ' ') << name << " = \n";
            if (expr)
                expr->print(os, spaces + 2);
        }
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;

    void declare(MM::MM& muncher) override {
        for (auto &[name, _] : var_inits) {
            muncher.scope().declare(name, type, muncher.new_temp());
        }
    }
};

struct ProcDecl : Declaration {
    std::string name;
    Lexer::Token return_type;
    std::vector<Param> params;
    std::unique_ptr<Block> block;

    ProcDecl(std::string name, Lexer::Token return_type, std::vector<Param> params, std::unique_ptr<Block> block) 
        : name(name), return_type(return_type), params(std::move(params)), block(std::move(block)) {}

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[Procedure] " << name;
        if (return_type.is_type(Lexer::INT) || return_type.is_type(Lexer::BOOL))
            os << " -> " << return_type.get_text();
        os << "\n";
        for (auto &param : params) {
            param.print(os, spaces + 1);
        }
        if (block)
            block->print(os, spaces + 1);
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;

    void declare(MM::MM& muncher) override {
        std::cout << "Declared " << name << " with type " << return_type.get_text() << "\n";
        muncher.scope().declare(name, MM::lexer_to_mm_type[return_type.get_type()], muncher.new_temp());
    }
};

/*
Program
*/
struct Program {
    std::vector<std::unique_ptr<Declaration>> declarations;

    Program(std::vector<std::unique_ptr<Declaration>> declarations) : declarations(std::move(declarations)) {}

    void print(std::ostream& os, int spaces = 0) {
        os << std::string(2 * spaces, ' ') << "[Program]\n";
        for (auto &declaration : declarations) {
            if (declaration)
                declaration->print(os, spaces + 1);
        }
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher);
};

}; // namespace AST