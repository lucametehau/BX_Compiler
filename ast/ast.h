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

    virtual void type_check(MM::MM& muncher) = 0;
};

template<typename T, typename = std::enable_if_t<std::is_base_of_v<AST, T>>> // wtf
inline std::ostream& operator << (std::ostream& os,  std::unique_ptr<AST>& ast) {
    if (ast)
        ast->print(os);
    return os;
}

/*
Type (AST)
*/

struct Type : AST {
    enum Kind { INT, BOOL, VOID, FUNCTION } kind;

    std::vector<std::unique_ptr<Type>> param_types;
    std::unique_ptr<Type> return_type;

    Type() = default;
    Type(Kind kind) : kind(kind) {}

    Type(const Type& other) : kind(other.kind) {
        param_types.reserve(other.param_types.size());
        for (const auto& p : other.param_types)
            param_types.push_back(std::make_unique<Type>(*p));
        if (other.return_type)
            return_type = std::make_unique<Type>(*other.return_type);
    }

    Type& operator=(const Type& other) {
        if (this != &other) {
            kind = other.kind;
            param_types.clear();
            param_types.reserve(other.param_types.size());
            for (const auto& p : other.param_types)
                param_types.push_back(std::make_unique<Type>(*p));
            if (other.return_type)
                return_type = std::make_unique<Type>(*other.return_type);
            else
                return_type = nullptr;
        }
        return *this;
    }

    static Type Int()  { return Type(INT); }
    static Type Bool() { return Type(BOOL); }
    static Type Void() { return Type(VOID); }

    static Type Function(std::vector<std::unique_ptr<Type>> params, std::unique_ptr<Type> ret) {
        Type t(FUNCTION);
        t.param_types.reserve(params.size());
        for (const auto& p : params)
            t.param_types.push_back(std::make_unique<Type>(*p));
        t.return_type = std::move(ret);
        return t;
    }

    [[nodiscard]] std::string to_string() const {
        switch (kind) {
            case INT: return "int";
            case BOOL: return "bool";
            case VOID: return "void";
            case FUNCTION: {
                std::string s = "function(";
                for (size_t i = 0; i < param_types.size(); i++) {
                    s += param_types[i]->to_string();
                    if (i + 1 < param_types.size()) s += ", ";
                }
                s += ") -> " + return_type->to_string();
                return s;
            }
        }
        return "<?>";
    }

    MM::Type to_mm_type() {
        switch (kind) {
            case INT: return MM::Type::Int();
            case BOOL: return MM::Type::Bool();
            case VOID: return MM::Type::Void();
            case FUNCTION: {
                std::vector<MM::Type> params;
                for (auto &p : param_types)
                    params.push_back(p->to_mm_type());
                return MM::Type::Function(params, return_type->to_mm_type());
            }
        }

        throw std::runtime_error(
            "Unexpected type kind!"
        );
    }

    constexpr bool is_first_order() const {
        return kind == INT || kind == BOOL || kind == VOID;
    }

    constexpr bool is_void() const {
        return kind == VOID;
    }

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[Type] " << to_string() << "\n";
    }

    std::vector<TAC> munch([[maybe_unused]] MM::MM& muncher) override {
        return {};
    }

    void type_check([[maybe_unused]] MM::MM& muncher) override {}
};


/*
Expressions
*/

struct Expression : AST {
protected:
    MM::Type type = MM::Type::Void();

public:
    void print(std::ostream& os, int spaces = 0) override = 0;

    [[nodiscard]] MM::Type get_type() { return type; }

    [[nodiscard]] virtual std::vector<TAC> munch(MM::MM& muncher) override = 0;
    [[nodiscard]] virtual std::vector<TAC> munch_bool([[maybe_unused]] MM::MM& muncher, 
                                                      [[maybe_unused]] std::string label_true, 
                                                      [[maybe_unused]] std::string label_false) {
        return {};
    }

    virtual void type_check(MM::MM& muncher) override = 0;
};

struct NumberExpression : Expression {
    int64_t value;

    NumberExpression(int64_t value) : value(value) {
        type = MM::Type::Int();
    }

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[NUMBER] " << value << "\n";
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;    

    void type_check([[maybe_unused]] MM::MM& muncher) override;
};

struct IdentExpression : Expression {
    std::string name;

    IdentExpression(std::string name) : name(name) {}

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[IDENT] " << name << "\n";
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;
    [[nodiscard]] std::vector<TAC> munch_bool([[maybe_unused]] MM::MM& muncher, std::string label_true, std::string label_false) override;    
  

    void type_check(MM::MM& muncher) override;
};

struct BoolExpression : Expression {
    bool value;

    BoolExpression(std::string s_value) {
        value = s_value == "true";
        type = MM::Type::Bool();
    }

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[BOOL] " << value << "\n";
    }

    [[nodiscard]] std::vector<TAC> munch([[maybe_unused]] MM::MM& muncher) override {
        return {};
    }

    [[nodiscard]] std::vector<TAC> munch_bool([[maybe_unused]] MM::MM& muncher, std::string label_true, std::string label_false) override;    

    void type_check([[maybe_unused]] MM::MM& muncher) override;
};

struct UniOpExpression : Expression {
    lexer::Token token;
    std::unique_ptr<Expression> expr;

    UniOpExpression(lexer::Token token, std::unique_ptr<Expression> _expr) : token(token), expr(std::move(_expr)) {
        type = expr->get_type();
    }

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[UniOp] " << token.get_text() << "\n";
        if (expr)
            expr->print(os, spaces + 1);
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;
    [[nodiscard]] std::vector<TAC> munch_bool(MM::MM& muncher, std::string label_true, std::string label_false) override;    

    void type_check(MM::MM& muncher) override;
};

struct BinOpExpression : Expression {
    lexer::Token token;
    std::unique_ptr<Expression> left, right;

    BinOpExpression(lexer::Token token, std::unique_ptr<Expression> _left, std::unique_ptr<Expression> _right) 
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

    void type_check(MM::MM& muncher) override;
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
    [[nodiscard]] std::vector<TAC> munch_bool(MM::MM& muncher, std::string label_true, std::string label_false) override;    

    void type_check(MM::MM& muncher) override;
};

/*
Statements
*/
struct Statement : AST {
    void print(std::ostream& os, int spaces = 0) override = 0;

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override = 0;    

    void type_check(MM::MM& muncher) override = 0;
};

struct Param {
    std::string name;
    std::unique_ptr<Type> declaration_type;
    MM::Type type;

    Param(std::string name, std::unique_ptr<Type> _declaration_type) 
        : name(name), declaration_type(std::move(_declaration_type)) {
        type = declaration_type->to_mm_type();
    }

    void print(std::ostream& os, int spaces = 0) {
        os << std::string(2 * spaces, ' ') << "[Param] " << name << " : " << declaration_type->to_string() << "\n";
    }

    // gets name and muncher type of parameter
    std::pair<std::string, MM::Type> get() {
        return make_pair(name, type);
    }

    void type_check([[maybe_unused]] MM::MM& muncher);  
};

struct VarDecl : Statement {
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> var_inits;
    MM::Type type;

    VarDecl(std::vector<std::pair<std::string, std::unique_ptr<Expression>>> var_inits, lexer::Token _type) 
        : var_inits(std::move(var_inits)), type(MM::lexer_to_mm_type[_type.get_type()]) {}

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[VarDecl] " << type.to_string() << "\n";
        for (auto &[name, expr] : var_inits) {
            os << std::string(2 * spaces + 2, ' ') << name << " = \n";
            if (expr)
                expr->print(os, spaces + 2);
        }
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;    

    void type_check(MM::MM& muncher) override;
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

    void type_check(MM::MM& muncher) override;
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

    void type_check(MM::MM& muncher) override;
};

struct Jump : Statement {
    lexer::Token token;

    Jump(lexer::Token token) : token(token) {}

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ');
        os << (token.is_type(lexer::CONTINUE) ? "[Continue]" : "[Break]");
        os << "\n";
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;    

    void type_check([[maybe_unused]] MM::MM& muncher) override {}
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

    void type_check(MM::MM& muncher) override;
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

    void type_check(MM::MM& muncher) override;
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

    void type_check(MM::MM& muncher) override;
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

    void type_check(MM::MM& muncher) override;
};

/*
Lambda
*/
struct Lambda : Statement {
    std::string name;
    std::unique_ptr<Type> return_type;
    std::vector<Param> params;
    std::unique_ptr<Block> block;

    Lambda(std::string name, std::unique_ptr<Type> return_type, std::vector<Param> params, std::unique_ptr<Block> block) 
        : name(name), return_type(std::move(return_type)), params(std::move(params)), block(std::move(block)) {}

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[Lambda] " << name << " -> " << return_type->to_string() << "\n";
        for (auto &param : params)
            param.print(os, spaces + 1);
        if (block)
            block->print(os, spaces + 1);
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;

    void type_check(MM::MM& muncher) override;
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

    GlobalVarDecl(std::vector<std::pair<std::string, std::unique_ptr<Expression>>> var_inits, lexer::Token _type) 
        : var_inits(std::move(var_inits)), type(MM::lexer_to_mm_type[_type.get_type()]) {}

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[VarDecl] " << type.to_string() << "\n";
        for (auto &[name, expr] : var_inits) {
            os << std::string(2 * spaces + 2, ' ') << name << " = \n";
            if (expr)
                expr->print(os, spaces + 2);
        }
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;

    void declare(MM::MM& muncher) override {
        for (auto &[name, _] : var_inits) {
            if (muncher.is_declared(name)) {
                throw std::runtime_error(std::format(
                    "Variable '{}' already declared in this scope!", name
                ));
            }
            muncher.scope().declare(name, type, "@" + name);
        }
    }    

    void type_check(MM::MM& muncher) override;
};

struct ProcDecl : Declaration {
    std::string name;
    std::unique_ptr<Type> return_type;
    std::vector<Param> params;
    std::unique_ptr<Block> block;

    ProcDecl(std::string name, std::unique_ptr<Type> return_type, std::vector<Param> params, std::unique_ptr<Block> block) 
        : name(name), return_type(std::move(return_type)), params(std::move(params)), block(std::move(block)) {}

    void print(std::ostream& os, int spaces = 0) override {
        os << std::string(2 * spaces, ' ') << "[Procedure] " << name << " -> " << return_type->to_string() << "\n";
        for (auto &param : params)
            param.print(os, spaces + 1);
        if (block)
            block->print(os, spaces + 1);
    }

    [[nodiscard]] std::vector<TAC> munch(MM::MM& muncher) override;

    void declare(MM::MM& muncher) override {
        if (muncher.is_declared(name)) {
            throw std::runtime_error(std::format(
                "Variable '{}' already declared in this scope!", name
            ));
        }

        if (name == "main" && !return_type->is_void()) {
            throw std::runtime_error(std::format(
                "Function main expected 'void' type!"
            ));
        }

        std::vector<MM::Type> param_types;
        for (std::size_t i = 0; i < params.size(); i++) {
#ifdef DEBUG
            std::cout << "Declared arg " << i << " with type " << params[i].type.to_string() << "\n";
#endif
            param_types.push_back(params[i].type);
        }

        MM::Type type = MM::Type::Function(param_types, return_type->to_mm_type());
        muncher.scope().declare(name, type, "#" + muncher.new_temp());
        // muncher.scope().declare(name + "$static_link", MM::Type::Int(), muncher.new_temp());

#ifdef DEBUG
        std::cout << "Declared " << name << " with type " << type.to_string() << "\n";
#endif
    }

    void type_check(MM::MM& muncher) override;
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

    void type_check(MM::MM& muncher);
};

}; // namespace AST