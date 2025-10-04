#include "ast.h"
#include <cassert>
#include <iostream>
#include <format>

namespace AST {

/*
Expressions
*/

[[nodiscard]] std::vector<TAC> NumberExpression::munch(MM::MM& muncher) {
    type = Type::INT;
    return {TAC(
        "const",
        { std::to_string(value) },
        muncher.new_temp()
    )};
}

[[nodiscard]] std::vector<TAC> IdentExpression::munch(MM::MM& muncher) {
    return {TAC(
        "copy",
        { muncher.get_temp(name) },
        muncher.new_temp()
    )};
}

[[nodiscard]] std::vector<TAC> BoolExpression::munch_bool([[maybe_unused]] MM::MM& muncher, std::string label_true, std::string label_false) {
    type = Type::BOOL;
    return {TAC(
        "jmp",
        {},
        value ? label_true : label_false
    )};
}

[[nodiscard]] std::vector<TAC> UniOpExpression::munch(MM::MM& muncher) {
    auto op = token.get_type();
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

    expr_munch.push_back(TAC(
        Lexer::op_code.find(op)->second,
        { expr_munch.back().get_result() },
        muncher.new_temp()
    ));

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
    std::vector<TAC> left_munch = left->munch(muncher);
    std::vector<TAC> right_munch = right->munch(muncher);

    // type checking
    if (left->get_type() != right->get_type()) {
        throw std::runtime_error(std::format(
            "Error at row {}, col {}! Binary Operator {} expected expressions of same types!", token.get_row(), token.get_col(), token.get_text()
        ));
    }
    if (Lexer::bool_binary_operators.find(op) != Lexer::bool_binary_operators.end()) {
        throw std::runtime_error(std::format(
            "Error at row {}, col {}! Expected 'int' type binary operator, got {}!", token.get_row(), token.get_col(), token.get_text()
        ));
    }
    else {
        if (left->get_type() != Type::INT) {
            throw std::runtime_error(std::format(
                "Error at row {}, col {}! Binary Operator {} expected type 'int' expression!", token.get_row(), token.get_col(), token.get_text()
            ));
        }
    }
    type = left->get_type();

    auto tl = left_munch.back().get_result(), tr = right_munch.back().get_result();

    for (auto &t : right_munch)
        left_munch.push_back(t);
    
    left_munch.push_back(TAC(
        Lexer::op_code.find(op)->second,
        { tl, tr },
        muncher.new_temp()
    ));
    return left_munch;
}


[[nodiscard]] std::vector<TAC> BinOpExpression::munch_bool(MM::MM& muncher, std::string label_true, std::string label_false) {
    auto op = token.get_type();

    std::vector<TAC> left_munch, right_munch;

    // short circuiting operators
    if (op == Lexer::ANDAND || op == Lexer::OROR)
    {
        auto label = muncher.new_label();
        left_munch = op == Lexer::ANDAND ? left->munch_bool(muncher, label, label_false)
                                         : left->munch_bool(muncher, label_true, label);
        right_munch = right->munch_bool(muncher, label_true, label_false);
        
        // type checking
        if (left->get_type() != right->get_type()) {
            throw std::runtime_error(std::format(
                "Error at row {}, col {}! Binary Operator {} expected expressions of same types!", token.get_row(), token.get_col(), token.get_text()
            ));
        }
        if (Lexer::bool_binary_operators.find(op) != Lexer::bool_binary_operators.end()) {
            if (left->get_type() != Type::BOOL) {
                throw std::runtime_error(std::format(
                    "Error at row {}, col {}! Binary Operator {} expected type 'bool' expression!", token.get_row(), token.get_col(), token.get_text()
                ));
            }
        }
        else {
            throw std::runtime_error(std::format(
                "Error at row {}, col {}! Expected 'bool' type binary operator!", token.get_row(), token.get_col()
            ));
        }
        type = Type::BOOL;
        
        left_munch.push_back(TAC(
            "label",
            { label }
        ));
        for (auto &t : right_munch)
            left_munch.push_back(t);
        return left_munch;
    }

    left_munch = left->munch(muncher);
    right_munch = right->munch(muncher);

    // type checking
    if (left->get_type() != right->get_type()) {
        throw std::runtime_error(std::format(
            "Error at row {}, col {}! Binary Operator {} expected expressions of same types!", token.get_row(), token.get_col(), token.get_text()
        ));
    }
    if (Lexer::bool_binary_operators.find(op) != Lexer::bool_binary_operators.end()) {
        if (left->get_type() != Type::INT) {
            throw std::runtime_error(std::format(
                "Error at row {}, col {}! Binary Operator {} expected type 'int' expression!", token.get_row(), token.get_col(), token.get_text()
            ));
        }
    }
    else {
        throw std::runtime_error(std::format(
            "Error at row {}, col {}! Expected 'bool' type binary operator, got {}!", token.get_row(), token.get_col(), token.get_text()
        ));
    }
    type = Type::BOOL;

    auto tl = left_munch.back().get_result(), tr = right_munch.back().get_result();

    for (auto &t : right_munch)
        left_munch.push_back(t);
    
    left_munch.push_back(TAC(
        "sub",
        { tl, tr },
        muncher.new_temp()
    ));

    left_munch.push_back(TAC(
        Lexer::jump_code.find(op)->second,
        { tl },
        label_true
    ));

    left_munch.push_back(TAC(
        "jmp",
        {},
        label_false
    ));

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

    muncher.scope().declare(name, expr_munch.back().get_result());
    return expr_munch;
}

[[nodiscard]] std::vector<TAC> Assign::munch(MM::MM& muncher) {
    std::vector<TAC> expr_munch = expr->munch(muncher);

    if (expr->get_type() != Type::INT)
        throw std::runtime_error("Expected Assign only for type 'int', since Variable Declaration is only available for 'int'!");
    
    expr_munch.push_back(TAC(
        "copy",
        { expr_munch.back().get_result() },
        muncher.get_temp(name)
    ));
    return expr_munch;
}

[[nodiscard]] std::vector<TAC> Print::munch(MM::MM& muncher) {
    std::vector<TAC> expr_munch = expr->munch(muncher);

    if (expr->get_type() != Type::INT)
        throw std::runtime_error("Expected Print only for type 'int'!");

    expr_munch.push_back(TAC(
        "print",
        { expr_munch.back().get_result() }
    ));
    return expr_munch;
}

[[nodiscard]] std::vector<TAC> Jump::munch(MM::MM& muncher) {
    return {TAC(
        "jmp",
        {},
        token.is_type(Lexer::BREAK) ? muncher.get_break_point() : muncher.get_continue_point()
    )};
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
    if (!else_branch.has_value()) {
        auto expr_munch = expr->munch_bool(muncher, label_then, label_end);
        if (expr->get_type() != Type::BOOL)
            throw std::runtime_error("Expected condition of type 'bool' in 'if'!");
        
        auto then_munch = then_branch->munch(muncher);
        
        expr_munch.push_back(TAC(
            "label", 
            { label_then }
        ));

        for (auto &t : then_munch)
            expr_munch.push_back(t);
        
        expr_munch.push_back(TAC(
            "label",
            { label_end }
        ));
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
    expr_munch.push_back(TAC(
        "label",
        { label_then }
    ));

    for (auto &t : then_munch)
        expr_munch.push_back(t);
    
    expr_munch.push_back(TAC(
        "jmp",
        {},
        label_end
    ));

    // else
    expr_munch.push_back(TAC(
        "label",
        { label_else }
    ));

    for (auto &t : else_munch)
        expr_munch.push_back(t);
    
    expr_munch.push_back(TAC(
        "label",
        { label_end }
    ));

    return expr_munch;
}

/*
While
*/
[[nodiscard]] std::vector<TAC> While::munch(MM::MM& muncher) {
    auto label_start = muncher.new_label();
    auto label_block = muncher.new_label();
    auto label_end = muncher.new_label();

    muncher.push_break_point(label_end);
    muncher.push_continue_point(label_start);

    std::vector<TAC> instr;

    instr.push_back(TAC(
        "label",
        { label_start }
    ));

    std::vector<TAC> expr_munch = expr->munch_bool(muncher, label_block, label_end);
    for (auto &t : expr_munch)
        instr.push_back(t);

    instr.push_back(TAC(
        "label",
        { label_block }
    ));

    std::vector<TAC> block_munch = block->munch(muncher);
    for (auto &t : block_munch)
        instr.push_back(t);

    instr.push_back(TAC(
        "jmp",
        {},
        label_start
    ));

    instr.push_back(TAC(
        "label",
        { label_end }
    ));

    muncher.pop_break_point();
    muncher.pop_continue_point();

    return instr;
}

/*
Program
*/
[[nodiscard]] std::vector<TAC> Program::munch(MM::MM& muncher) {
    std::vector<TAC> instr;
    instr.push_back(TAC(
        "label",
        { muncher.new_label() }
    ));
    
    std::vector<TAC> block_instr = block->munch(muncher);
    for (auto &t : block_instr)
        instr.push_back(t);
    
    instr.push_back(TAC(
        "const",
        { std::to_string(0) },
        muncher.new_temp()
    ));
    instr.push_back(TAC(
        "ret",
        { instr.back().get_result() }
    ));
    return instr;
}


}; // namespace AST