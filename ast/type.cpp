#include "ast.h"
#include "../utils/utils.h"
#include <cassert>
#include <iostream>
#include <format>

namespace AST {

/*
Expressions
*/

void NumberExpression::type_check([[maybe_unused]] MM::MM& muncher) {
    type = MM::Type::Int();
}

void IdentExpression::type_check(MM::MM& muncher) {
    type = muncher.get_type(name);
}

void BoolExpression::type_check([[maybe_unused]] MM::MM& muncher) {
    type = MM::Type::Bool();
}

void UniOpExpression::type_check(MM::MM& muncher) {
    auto op = token.get_text();
    expr->type_check(muncher);
    
    // type checking
    if (op == "!") {
        type = MM::Type::Bool();
        if (!expr->get_type().is_bool()) {
            throw std::runtime_error(std::format(
                "Error at row {}, col {}! Unary operator {} expected type 'bool' expression!", token.get_row(), token.get_col(), op
            ));
        }
    }
    else {
        type = MM::Type::Int();
        if (!expr->get_type().is_int()) {
            throw std::runtime_error(std::format(
                "Error at row {}, col {}! Unary operator {} expected type 'int' expression!", token.get_row(), token.get_col(), op
            ));
        }
    }
}

void BinOpExpression::type_check(MM::MM& muncher) {
    auto op = token.get_type();
    left->type_check(muncher);
    right->type_check(muncher);

    // type checking
    if (left->get_type() != right->get_type()) {
        throw std::runtime_error(std::format(
            "Error at row {}, col {}! Binary Operator {} expected expressions of same types!", token.get_row(), token.get_col(), token.get_text()
        ));
    }
    if (lexer::bool_binary_operators.find(op) != lexer::bool_binary_operators.end()) {
        type = MM::Type::Bool();
        if (op == lexer::ANDAND || op == lexer::OROR) {
            if (!left->get_type().is_bool()) {
                throw std::runtime_error(std::format(
                    "Error at row {}, col {}! Expected type 'bool' terms for binary operator {}, got type {}!", token.get_row(), token.get_col(), token.get_text(), left->get_type().to_string()
                ));
            }
        }
        else {
            if (!left->get_type().is_int()) {
                throw std::runtime_error(std::format(
                    "Error at row {}, col {}! Expected type 'int' terms for binary operator {}, got type {}!", token.get_row(), token.get_col(), token.get_text(), left->get_type().to_string()
                ));
            }
        }
    }
    else {
        type = MM::Type::Int();
        if (!left->get_type().is_int()) {
            throw std::runtime_error(std::format(
                "Error at row {}, col {}! Expected type 'int' terms for binary operator {}, got type {}!", 
                token.get_row(), token.get_col(), token.get_text(), left->get_type().to_string()
            ));
        }
    }
}

/*
Function/Procedure evaluation
*/


void Eval::type_check(MM::MM& muncher) {
    std::size_t param_count = 0;

    // print is a special function
    if (name != "print")
        type = muncher.get_type(name).get_return_type();
    else {
        type = MM::Type::Void();
    }

    for (auto &expr : params) {
        expr->type_check(muncher);

#ifdef DEBUG
        std::cout << name << " " << param_count << " " << expr->get_type().to_string() << "\n";
#endif

        if (name != "print") {
            auto arg_type = muncher.get_type(name).get_param_type(param_count);
            if (arg_type != expr->get_type()) {
                throw std::runtime_error(std::format(
                    "Expected arguments of type {}, got argument of type {} for function '{}'!", arg_type.to_string(), expr->get_type().to_string(), name
                ));
            }
        }
        else {
            if (!expr->get_type().is_bool() && !expr->get_type().is_int()) {
                throw std::runtime_error(std::format(
                    "'print' expected arguments of type 'int' or 'bool', got argument of type '{}'!", expr->get_type().to_string()
                ));
            }
        }

        param_count++;
    }
}

/*
Statements
*/

void VarDecl::type_check(MM::MM& muncher) {
    for (auto &[name, expr] : var_inits) {
        expr->type_check(muncher);

        if (expr->get_type() != type) {
            throw std::runtime_error(std::format(
                "Expected variable declaration of type {}, got type {}!", type.to_string(), expr->get_type().to_string()
            ));
        }
        muncher.scope().declare(name, type, muncher.new_temp());
    }
}

void Assign::type_check(MM::MM& muncher) {
    auto temp = muncher.get_temp(name);
    auto var_type = muncher.get_type(name);

    expr->type_check(muncher);

    if (expr->get_type() != var_type) {
        throw std::runtime_error(std::format(
            "Expected Variable Declaration of type {}, got type {}!", var_type.to_string(), expr->get_type().to_string()
        ));
    }
}

void Call::type_check(MM::MM& muncher) {
    eval->type_check(muncher);

    if (!eval->get_type().is_void()) {
        std::cout << std::format(
            "Warning! Called procedure of type '{}' without using the result!", eval->get_type().to_string()
        ) << "\n";
    }
}

/*
Block
*/
void Block::type_check(MM::MM& muncher) {
    muncher.push_scope();
    for (auto &stmt : statements) {
        stmt->type_check(muncher);
    }
    muncher.pop_scope();
}

/*
If Else
*/
void IfElse::type_check(MM::MM& muncher) {
    if (!else_branch.has_value()) {
        expr->type_check(muncher);
        if (!expr->get_type().is_bool()) {
            throw std::runtime_error(std::format(
                "Expected condition of type 'bool' in 'if', got type '{}'!", expr->get_type().to_string()
            ));
        }
        
        then_branch->type_check(muncher);

        return;
    }

    expr->type_check(muncher);
    if (!expr->get_type().is_bool()) {
        throw std::runtime_error(std::format(
            "Expected condition of type 'bool' in 'if', got type '{}'!", expr->get_type().to_string()
        ));
    }
    
    then_branch->type_check(muncher);
    else_branch.value()->type_check(muncher);
}

/*
While
*/
void While::type_check(MM::MM& muncher) {
    muncher.push_break_point("0");
    muncher.push_continue_point("0");

    expr->type_check(muncher);

    if (!expr->get_type().is_bool()) {
        throw std::runtime_error(std::format(
            "Expected while condition of type 'bool', got type '{}'", expr->get_type().to_string()
        ));
    }

    block->type_check(muncher);

    muncher.pop_break_point();
    muncher.pop_continue_point();
}

/*
Lambdas
*/

void Lambda::type_check(MM::MM& muncher) {
    // create new scope for the lambda
    muncher.push_scope();
    muncher.scope().set_function(name, return_type->to_mm_type());

    for (auto &param : params) {
        auto [name, type] = param.get();
        muncher.scope().declare(name, type, muncher.new_param_temp());
    }

    block->type_check(muncher);
    muncher.pop_scope();
    
    // declare the lambda, only after processing its content
    std::vector<MM::Type> param_types;
    for (auto &param : params)
        param_types.push_back(param.type);

    auto lambda_type = MM::Type::Function(param_types, return_type->to_mm_type());
#ifdef DEBUG
    std::cout << "Lambda " << name << " has type " << lambda_type.to_string() << "\n";
#endif
    muncher.scope().declare(name, lambda_type, muncher.new_temp());
}

/*
Declarations
*/

void GlobalVarDecl::type_check(MM::MM& muncher) {
    for (auto &[name, expr] : var_inits) {
        expr->type_check(muncher);

        if (expr->get_type() != type) {
            throw std::runtime_error(std::format(
                "Expected Variable Declaration of type {}, got type {}!", type.to_string(), expr->get_type().to_string()
            ));
        }
    }
}

void ProcDecl::type_check(MM::MM& muncher) {
    muncher.scope().set_function(name, return_type->to_mm_type());

    for (auto &param : params) {
        auto [name, type] = param.get();
        muncher.scope().declare(name, type, muncher.new_param_temp());
    }

    block->type_check(muncher);
}

void Return::type_check(MM::MM& muncher) {
    if (!expr) {
        return;
    }
    
    // we have a return expression
    // compare with current function's type
    auto type = muncher.get_curr_function_type();
    expr->type_check(muncher);

    if (type != expr->get_type()) {
        throw std::runtime_error(std::format(
            "Return has wrong type! Expected '{}', got '{}'!", type.to_string(), expr->get_type().to_string()
        ));
    }
}

/*
Program
*/
void Program::type_check(MM::MM& muncher) {
    std::vector<TAC> instr;
    
    // global scope
    muncher.push_scope();

    for (auto &declaration : declarations)
        declaration->declare(muncher);
    
    for (auto &declaration : declarations) {
        declaration->type_check(muncher);
    }

    muncher.pop_scope();
}


}; // namespace AST