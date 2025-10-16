#include "ast.h"
#include "../utils/utils.h"
#include <cassert>
#include <iostream>
#include <format>

namespace AST {

/*
Expressions
*/

[[nodiscard]] std::vector<TAC> NumberExpression::munch(MM::MM& muncher) {
    type = MM::Type::INT;
    return {TAC(
        "const",
        { std::to_string(value) },
        muncher.new_temp()
    )};
}

[[nodiscard]] std::vector<TAC> IdentExpression::munch(MM::MM& muncher) {
    type = muncher.get_type(name);
    return {TAC(
        "copy",
        { muncher.get_temp(name) },
        muncher.new_temp()
    )};
}

[[nodiscard]] std::vector<TAC> BoolExpression::munch_bool([[maybe_unused]] MM::MM& muncher, std::string label_true, std::string label_false) {
    type = MM::Type::BOOL;
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
        // if (expr->get_type() != MM::Type::BOOL)
        //     throw std::runtime_error("Unary operator " + op + " expected type 'bool' expression!");
    }
    else {
        if (expr->get_type() != MM::Type::INT)
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
        if (expr->get_type() != MM::Type::BOOL)
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
        if (left->get_type() != MM::Type::INT) {
            throw std::runtime_error(std::format(
                "Error at row {}, col {}! Binary Operator {} expected type 'int' expression!", token.get_row(), token.get_col(), token.get_text()
            ));
        }
    }
    type = left->get_type();

    auto tl = left_munch.back().get_result(), tr = right_munch.back().get_result();

    utils::concat(left_munch, right_munch);
    
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
            if (left->get_type() != MM::Type::BOOL) {
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
        type = MM::Type::BOOL;
        
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
        if (left->get_type() != MM::Type::INT) {
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
    type = MM::Type::BOOL;

    auto tl = left_munch.back().get_result(), tr = right_munch.back().get_result();

    utils::concat(left_munch, right_munch);
    
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
Function/Procedure evaluation
*/


[[nodiscard]] std::vector<TAC> Eval::munch(MM::MM& muncher) {
    std::vector<TAC> instr;
    std::size_t param_count = 1;

    // print is a special function
    if (name != "print")
        type = muncher.get_type(name);
    else
        type = MM::Type::NONE;

    for (auto &expr : params) {
        auto expr_munch = expr->munch(muncher);

        utils::concat(instr, expr_munch);

        instr.push_back(TAC(
            "param",
            { expr_munch.back().get_result() },
            std::to_string(param_count)
        ));

        param_count++;
    }

    if (type == MM::Type::NONE) {
        instr.push_back(TAC(
            "call",
            { "@" + name, std::to_string(params.size()) }
        ));
    }
    else {
        instr.push_back(TAC(
            "call",
            { "@" + name, std::to_string(params.size()) },
            muncher.new_temp()
        ));
    }

    return instr;
}

/*
Statements
*/

[[nodiscard]] std::vector<TAC> VarDecl::munch(MM::MM& muncher) {
    std::vector<TAC> instr;
    for (auto &[name, expr] : var_inits) {
        auto expr_munch = expr->munch(muncher);

        if (expr->get_type() != type) {
            throw std::runtime_error(std::format(
                "Expected Variable Declaration of type {}, got type {}!", MM::type_text[type], MM::type_text[expr->get_type()]
            ));
        }

        utils::concat(instr, expr_munch);
        muncher.scope().declare(name, type, muncher.new_temp());
    }

    return instr;
}

[[nodiscard]] std::vector<TAC> Assign::munch(MM::MM& muncher) {
    auto temp = muncher.get_temp(name);
    auto var_type = muncher.get_type(name);

    if (var_type == MM::Type::INT) {
        auto expr_munch = expr->munch(muncher);

        if (expr->get_type() != var_type) {
            throw std::runtime_error(std::format(
                "Expected Variable Declaration of type {}, got type {}!", MM::type_text[var_type], MM::type_text[expr->get_type()]
            ));
        }
        
        expr_munch.push_back(TAC(
            "copy",
            { expr_munch.back().get_result() },
            temp
        ));
        return expr_munch;
    }

    // bool assignment

    auto label_true = muncher.new_label();
    auto label_false = muncher.new_label();
    auto label_end = muncher.new_label();

    auto expr_munch = expr->munch_bool(muncher, label_true, label_false);

    expr_munch.push_back(TAC(
        "label",
        { label_true }
    ));

    expr_munch.push_back(TAC(
        "const",
        { "1" },
        temp
    ));

    expr_munch.push_back(TAC(
        "jmp",
        {},
        label_end
    ));

    expr_munch.push_back(TAC(
        "label",
        { label_false }
    ));

    expr_munch.push_back(TAC(
        "const",
        { "0" },
        temp
    ));

    expr_munch.push_back(TAC(
        "label",
        { label_end }
    ));

    return expr_munch;
}

[[nodiscard]] std::vector<TAC> Call::munch(MM::MM& muncher) {
    auto instr = eval->munch(muncher);

    if (eval->get_type() != MM::Type::NONE) {
        std::cout << std::format(
            "Warning! Expected type 'void' for procedure, got function of type '{}'!", MM::type_text[eval->get_type()]
        ) << "\n";
    }

    return instr;
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
        utils::concat(instr, stmt_munch);
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
        if (expr->get_type() != MM::Type::BOOL)
            throw std::runtime_error("Expected condition of type 'bool' in 'if'!");
        
        auto then_munch = then_branch->munch(muncher);
        
        expr_munch.push_back(TAC(
            "label", 
            { label_then }
        ));

        utils::concat(expr_munch, then_munch);
        
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
    if (expr->get_type() != MM::Type::BOOL)
        throw std::runtime_error("Expected condition of type 'bool' in 'if'!");
    
    auto then_munch = then_branch->munch(muncher);
    auto else_munch = else_branch.value()->munch(muncher);

    // if
    expr_munch.push_back(TAC(
        "label",
        { label_then }
    ));

    utils::concat(expr_munch, then_munch);
    
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

    utils::concat(expr_munch, else_munch);
    
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

    utils::concat(instr, expr_munch);

    instr.push_back(TAC(
        "label",
        { label_block }
    ));

    std::vector<TAC> block_munch = block->munch(muncher);

    utils::concat(instr, block_munch);

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
Procedures
*/
[[nodiscard]] std::vector<TAC> ProcDecl::munch(MM::MM& muncher) {
    std::vector<TAC> instr;
    std::vector<std::string> args;

    // function already declared in Program::munch
    muncher.scope().declare(name, MM::lexer_to_mm_type[return_type.get_type()], muncher.new_temp());

    for (auto &param : params) {
        auto [name, type] = param;
        muncher.scope().declare(name, MM::lexer_to_mm_type[type.get_type()], muncher.new_param_temp());
        args.push_back(name);
    }

    instr.push_back(TAC(
        "proc",
        args,
        name
    ));

    auto block_munch = block->munch(muncher);

    utils::concat(instr, block_munch);

    return instr;
}

[[nodiscard]] std::vector<TAC> Return::munch(MM::MM& muncher) {
    std::vector<TAC> instr;
    std::vector<std::string> args;
    
    if (expr) {
        instr = expr->munch(muncher);
        args = { instr.back().get_result() };
    }

    instr.push_back(TAC(
        "ret",
        args
    ));

    return instr;
}

/*
Program
*/
[[nodiscard]] std::vector<TAC> Program::munch(MM::MM& muncher) {
    std::vector<TAC> instr;
    
    // global scope
    muncher.push_scope();
    
    for (auto &declaration : declarations) {
        auto decl_munch = declaration->munch(muncher);
        utils::concat(instr, decl_munch);
    }

    return instr;
}


}; // namespace AST