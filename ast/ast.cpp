#include "ast.h"
#include <cassert>
#include <iostream>

namespace AST {

/*
Expressions
*/

[[nodiscard]] std::vector<TAC> NumberExpression::munch(MM::MM& muncher) {
    TAC tac;
    tac["opcode"] = {"const"};
    tac["args"] = { std::to_string(value) };
    tac["result"] = { muncher.new_temp() };
    type = Type::INT;
    return {tac};
}

[[nodiscard]] std::vector<TAC> IdentExpression::munch(MM::MM& muncher) {
    TAC tac;
    tac["opcode"] = { "copy" };
    tac["args"] = { muncher.get_temp(name) };
    tac["result"] = { muncher.new_temp() };
    return {tac};
}

[[nodiscard]] std::vector<TAC> BoolExpression::munch_bool([[maybe_unused]] MM::MM& muncher, std::string label_true, std::string label_false) {
    TAC tac;
    tac["opcode"] = { "jmp"};
    tac["args"] = {};
    tac["result"] = { value ? label_true : label_false };
    type = Type::BOOL;
    return {tac};
}

[[nodiscard]] std::vector<TAC> UniOpExpression::munch(MM::MM& muncher) {
    auto op = token.get_type();
    TAC tac;
    std::vector<TAC> expr_munch = expr->munch(muncher);
    

    // type checking
    if (Lexer::bool_binary_operators.find(op) != Lexer::bool_binary_operators.end()) {
        throw std::runtime_error("Expected 'int' type unary operator, got 'bool' operator!");
        // if (expr->get_type() != Type::BOOL)
        //     throw std::runtime_error("Unary operator " + op + " expected type 'bool' expression!");
    }
    else {
        if (expr->get_type() != Type::INT)
            throw std::runtime_error("Unary operator " + token.get_text() + " expected type 'int' expression!");
    }
    type = expr->get_type();

    tac["opcode"] = { Lexer::op_code.find(op)->second };
    tac["args"] = { expr_munch.back()["result"][0] }; // expression result stored in this
    tac["result"] = { muncher.new_temp() };
    expr_munch.push_back(tac);

    return expr_munch;
}

[[nodiscard]] std::vector<TAC> UniOpExpression::munch_bool(MM::MM& muncher, std::string label_true, std::string label_false) {
    auto op = token.get_text();
    std::vector<TAC> expr_munch = expr->munch_bool(muncher, label_false, label_true);

    // type checking
    if (op == "!") {
        if (expr->get_type() != Type::BOOL)
            throw std::runtime_error("Unary operator " + op + " expected type 'bool' expression!");
    }
    else {
        throw std::runtime_error("Expected 'bool' type unary operator!");
    }
    type = expr->get_type();

    return expr_munch;
}

[[nodiscard]] std::vector<TAC> BinOpExpression::munch(MM::MM& muncher) {
    auto op = token.get_type();
    TAC tac;
    std::vector<TAC> left_munch = left->munch(muncher);
    std::vector<TAC> right_munch = right->munch(muncher);

    // type checking
    if (left->get_type() != right->get_type()) {
        throw std::runtime_error("Binary Operator expected expressions of same types!");
    }
    if (Lexer::bool_binary_operators.find(op) != Lexer::bool_binary_operators.end()) {
        throw std::runtime_error("Expected 'int' type binary operator, got 'bool' operator!");
    }
    else {
        if (left->get_type() != Type::INT)
            throw std::runtime_error("Binary operator " + token.get_text() + " expected type 'int' expression!");
    }
    type = left->get_type();

    tac["opcode"] = { Lexer::op_code.find(op)->second };
    tac["args"] = { left_munch.back()["result"][0], right_munch.back()["result"][0] }; // expression result stored in this
    tac["result"] = { muncher.new_temp() };

    for (auto &t : right_munch)
        left_munch.push_back(t);
    left_munch.push_back(tac);
    return left_munch;
}


[[nodiscard]] std::vector<TAC> BinOpExpression::munch_bool(MM::MM& muncher, std::string label_true, std::string label_false) {
    auto op = token.get_type();

    std::vector<TAC> left_munch, right_munch;
    TAC tac;

    // short circuiting operators
    if (op == Lexer::ANDAND || op == Lexer::OROR)
    {
        auto label = muncher.new_label();
        left_munch = op == Lexer::ANDAND ? left->munch_bool(muncher, label, label_false)
                                         : left->munch_bool(muncher, label_true, label);
        right_munch = right->munch_bool(muncher, label_true, label_false);
        
        // type checking
        if (left->get_type() != right->get_type()) {
            throw std::runtime_error("Binary Operator expected expressions of same types!");
        }
        if (Lexer::bool_binary_operators.find(op) != Lexer::bool_binary_operators.end()) {
            if (left->get_type() != Type::BOOL)
                throw std::runtime_error("Binary operator " + token.get_text() + " expected type 'bool' expression!");
        }
        else {
            throw std::runtime_error("Expected 'bool' type binary operator!");
        }
        type = Type::BOOL;
        
        tac["opcode"] = { "label" };
        tac["args"] = { label };
        tac["result"] = {};
        left_munch.push_back(tac);
        for (auto &t : right_munch)
            left_munch.push_back(t);
        return left_munch;
    }

    left_munch = left->munch(muncher);
    right_munch = right->munch(muncher);

    // type checking
    if (left->get_type() != right->get_type()) {
        throw std::runtime_error("Binary Operator expected expressions of same types!");
    }
    if (Lexer::bool_binary_operators.find(op) != Lexer::bool_binary_operators.end()) {
        if (left->get_type() != Type::INT)
            throw std::runtime_error("Binary operator " + token.get_text() + " expected type 'int' expression!");
    }
    else {
        throw std::runtime_error("Expected 'bool' type binary operator!");
    }
    type = Type::BOOL;

    auto tl = left_munch.back()["result"][0], tr = right_munch.back()["result"][0];

    for (auto &t : right_munch)
        left_munch.push_back(t);
    
    tac["opcode"] = { "sub" };
    tac["args"] = { tl, tr };
    tac["result"] = { tl };
    left_munch.push_back(tac);

    tac["opcode"] = { Lexer::jump_code.find(op)->second };
    tac["args"] = { tl };
    tac["result"] = { label_true };
    left_munch.push_back(tac);

    tac["opcode"] = { "jmp" };
    tac["args"] = {};
    tac["result"] = { label_false };
    left_munch.push_back(tac);

    return left_munch;
}

/*
Statements
*/

[[nodiscard]] std::vector<TAC> VarDecl::munch(MM::MM& muncher) {
    
    std::vector<TAC> expr_munch = expr->munch(muncher);
    if (muncher.is_declared(name))
        throw std::runtime_error("Variable " + name + " already declared\n");
        
    if (expr->get_type() != Type::INT)
        throw std::runtime_error("Expected Variable Declaration only for type 'int'!");

    assert (!expr_munch.back()["result"].empty());
    muncher.scope().declare(name, expr_munch.back()["result"][0]);
    return expr_munch;
}

[[nodiscard]] std::vector<TAC> Assign::munch(MM::MM& muncher) {
    TAC tac;
    std::vector<TAC> expr_munch = expr->munch(muncher);

    if (expr->get_type() != Type::INT)
        throw std::runtime_error("Expected Assign only for type 'int', since Variable Declaration is only available for 'int'!");
    
    tac["opcode"] = { "copy" };
    tac["args"] = { expr_munch.back()["result"][0] };
    tac["result"] = { muncher.get_temp(name) };
    expr_munch.push_back(tac);
    return expr_munch;
}

[[nodiscard]] std::vector<TAC> Print::munch(MM::MM& muncher) {
    
    TAC tac;
    std::vector<TAC> expr_munch = expr->munch(muncher);

    if (expr->get_type() != Type::INT)
        throw std::runtime_error("Expected Print only for type 'int'!");

    tac["opcode"] = { "print" };
    tac["args"] = { expr_munch.back()["result"][0] };
    tac["result"] = { };
    expr_munch.push_back(tac);
    return expr_munch;
}

/*
Block
*/
[[nodiscard]] std::vector<TAC> Block::munch(MM::MM& muncher) {
    std::vector<TAC> instr;

    muncher.push_scope();
    for (auto &stmt : statements) {
        auto stmt_munch = stmt->munch(muncher);
        for (auto &t : stmt_munch)
            instr.push_back(t);
    }
    muncher.pop_scope();

    return instr;
}

/*
If Else
*/
[[nodiscard]] std::vector<TAC> IfElse::munch(MM::MM& muncher) {
    auto label_then = muncher.new_label();
    auto label_end = muncher.new_label();
    TAC tac;
    if (!else_branch.has_value()) {
        auto expr_munch = expr->munch_bool(muncher, label_then, label_end);
        if (expr->get_type() != Type::BOOL)
            throw std::runtime_error("Expected condition of type 'bool' in 'if'!");
        
        auto then_munch = then_branch->munch(muncher);
        tac["opcode"] = { "label" };
        tac["args"] = { label_then };
        tac["result"] = {};
        expr_munch.push_back(tac);
        for (auto &t : then_munch)
            expr_munch.push_back(t);
        tac["opcode"] = { "label" };
        tac["args"] = { label_end };
        tac["result"] = {};
        expr_munch.push_back(tac);
        return expr_munch;
    }

    auto label_else = muncher.new_label();

#ifdef DEBUG
    std::cout << "then: " << label_then << ", end: " << label_end << ", else: " << label_else << "\n";
#endif

    auto expr_munch = expr->munch_bool(muncher, label_then, label_else);
    if (expr->get_type() != Type::BOOL)
        throw std::runtime_error("Expected condition of type 'bool' in 'if'!");
    
    auto then_munch = then_branch->munch(muncher);
    auto else_munch = else_branch.value()->munch(muncher);

    // if
    tac["opcode"] = { "label" };
    tac["args"] = { label_then };
    tac["result"] = {};
    expr_munch.push_back(tac);

    for (auto &t : then_munch)
        expr_munch.push_back(t);
    
    tac["opcode"] = { "jmp" };
    tac["args"] = { label_end };
    tac["result"] = {};
    expr_munch.push_back(tac);

    // else
    tac["opcode"] = { "label" };
    tac["args"] = { label_else };
    tac["result"] = {};
    expr_munch.push_back(tac);

    for (auto &t : else_munch)
        expr_munch.push_back(t);
    
    tac["opcode"] = { "label" };
    tac["args"] = { label_end };
    tac["result"] = {};
    expr_munch.push_back(tac);

    return expr_munch;
}

/*
Program
*/
[[nodiscard]] std::vector<TAC> Program::munch(MM::MM& muncher) {
    return block->munch(muncher);
}


}; // namespace AST