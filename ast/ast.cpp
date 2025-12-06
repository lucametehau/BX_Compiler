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

[[nodiscard]] std::vector<TAC> IdentExpression::munch_bool(MM::MM& muncher, std::string label_true, std::string label_false) {
    std::vector<TAC> instr;

    instr.push_back(TAC(
        "jz",
        { muncher.get_temp(name) },
        label_false 
    ));

    instr.push_back(TAC(
        "jmp",
        {},
        label_true 
    ));

    return instr;
}

[[nodiscard]] std::vector<TAC> BoolExpression::munch_bool([[maybe_unused]] MM::MM& muncher, std::string label_true, std::string label_false) {
    return {TAC(
        "jmp",
        {},
        value ? label_true : label_false
    )};
}

[[nodiscard]] std::vector<TAC> UniOpExpression::munch(MM::MM& muncher) {
    auto op = token.get_type();
    std::vector<TAC> expr_munch = expr->munch(muncher);

    expr_munch.push_back(TAC(
        op == lexer::DASH ? "neg" : lexer::op_code.find(op)->second,
        { expr_munch.back().get_result() },
        muncher.new_temp()
    ));

    return expr_munch;
}

[[nodiscard]] std::vector<TAC> UniOpExpression::munch_bool(MM::MM& muncher, std::string label_true, std::string label_false) {
    return expr->munch_bool(muncher, label_false, label_true);
}

[[nodiscard]] std::vector<TAC> BinOpExpression::munch(MM::MM& muncher) {
    auto op = token.get_type();
    std::vector<TAC> left_munch = left->munch(muncher);
    std::vector<TAC> right_munch = right->munch(muncher);

    auto tl = left_munch.back().get_result(), tr = right_munch.back().get_result();

    utils::concat(left_munch, right_munch);
    
    left_munch.push_back(TAC(
        lexer::op_code.find(op)->second,
        { tl, tr },
        muncher.new_temp()
    ));
    return left_munch;
}


[[nodiscard]] std::vector<TAC> BinOpExpression::munch_bool(MM::MM& muncher, std::string label_true, std::string label_false) {
    auto op = token.get_type();

    std::vector<TAC> left_munch, right_munch;

    // short circuiting operators
    if (op == lexer::ANDAND || op == lexer::OROR)
    {
        auto label = muncher.new_label();
        left_munch = op == lexer::ANDAND ? left->munch_bool(muncher, label, label_false)
                                         : left->munch_bool(muncher, label_true, label);
        right_munch = right->munch_bool(muncher, label_true, label_false);
        
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

    auto tl = left_munch.back().get_result(), tr = right_munch.back().get_result();

    utils::concat(left_munch, right_munch);
    
    auto res_temp = muncher.new_temp();
    left_munch.push_back(TAC(
        "sub",
        { tl, tr },
        res_temp
    ));

    left_munch.push_back(TAC(
        lexer::jump_code.find(op)->second,
        { res_temp },
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
    std::vector<std::string> param_temps;
    std::size_t param_count = 1;

    if (name == "print") {
        if (params[0]->get_type().is_bool())
            name = "__bx_print_bool";
        else
            name = "__bx_print_int";
    }

    for (auto &expr : params) {
        auto arg_type = expr->get_type();

        if (arg_type.is_int()) {
            auto expr_munch = expr->munch(muncher);
            auto result_temp = expr_munch.back().get_result();

            utils::concat(instr, expr_munch);
            param_temps.push_back(result_temp);
            param_count++;
        }
        else if (arg_type.is_bool()) {
            auto label_true = muncher.new_label();
            auto label_false = muncher.new_label();
            auto label_end = muncher.new_label();
            auto expr_munch = expr->munch_bool(muncher, label_true, label_false);
            auto result_temp = muncher.new_temp();

            utils::concat(instr, expr_munch);

            instr.push_back(TAC(
                "label",
                { label_true }
            ));

            instr.push_back(TAC(
                "const",
                { "1" },
                result_temp
            ));

            instr.push_back(TAC(
                "jmp",
                {},
                label_end
            ));

            instr.push_back(TAC(
                "label",
                { label_false }
            ));

            instr.push_back(TAC(
                "const",
                { "0" },
                result_temp
            ));

            instr.push_back(TAC(
                "label",
                { label_end }
            ));

            param_temps.push_back(result_temp);
            param_count++;
        }
        else if (arg_type.is_function()) {
            std::string func_name = dynamic_cast<IdentExpression*>(expr.get())->name;
            std::string code_pointer = muncher.new_temp();
            std::string static_link = muncher.new_temp();

            if (muncher.get_temp(func_name)[0] == '#') {
                instr.push_back(TAC(
                    "const",
                    { func_name },
                    code_pointer
                ));

                instr.push_back(TAC(
                    "const",
                    { "0" },
                    static_link
                ));
            }
            else {
                instr.push_back(TAC(
                    "copy",
                    { muncher.get_temp(func_name) },
                    code_pointer
                ));

                instr.push_back(TAC(
                    "copy",
                    { muncher.get_temp(func_name + "$static_link") },
                    static_link
                ));
            }

            param_temps.push_back(code_pointer);
            param_temps.push_back(static_link);
            param_count += 2;
        }
    }

    std::string code_pointer;
    std::string static_link;

    // not a global function call
    if (muncher.is_defined(name) && muncher.get_temp(name)[0] == '%') {
        code_pointer = muncher.new_temp();
        static_link = muncher.new_temp();

        instr.push_back(TAC(
            "copy",
            { muncher.get_temp(name) },
            code_pointer
        ));

        instr.push_back(TAC(
            "copy",
            { muncher.get_temp(name + "$static_link") },
            static_link
        ));
    }
    // normal function call
    else {
        code_pointer = muncher.new_temp();
        static_link = muncher.new_temp();

        instr.push_back(TAC(
            "const",
            { name },
            code_pointer
        ));

        instr.push_back(TAC(
            "const", 
            { "0" }, 
            static_link
        ));

        instr.push_back(TAC(
            "copy", 
            { static_link }, 
            muncher.new_temp()
        ));
    }

    // add params in reverse order
    reverse(param_temps.begin(), param_temps.end());

    param_count++;

    // static link is last parameter
    instr.push_back(TAC(
        "param",
        { static_link },
        std::to_string(--param_count)
    ));

    // set args only after processing all
    for (auto &temp : param_temps) {
        instr.push_back(TAC(
            "param",
            { temp },
            std::to_string(--param_count)
        ));
    }

    if (type.is_void()) {
        instr.push_back(TAC(
            "call",
            { code_pointer, std::to_string(params.size() + 1) }
        ));
    }
    else {
        instr.push_back(TAC(
            "call",
            { code_pointer, std::to_string(params.size() + 1) },
            muncher.new_temp()
        ));
    }

    return instr;
}

[[nodiscard]] std::vector<TAC> Eval::munch_bool(MM::MM& muncher, std::string label_true, std::string label_false) {
    std::vector<TAC> instr;
    std::vector<std::string> param_temps;
    std::size_t param_count = 1;

    for (auto &expr : params) {
        auto arg_type = expr->get_type();

        if (arg_type.is_int()) {
            auto expr_munch = expr->munch(muncher);
            auto result_temp = expr_munch.back().get_result();

            utils::concat(instr, expr_munch);
            param_temps.push_back(result_temp);
            param_count++;
        }
        else if (arg_type.is_bool()) {
            auto label_true = muncher.new_label();
            auto label_false = muncher.new_label();
            auto label_end = muncher.new_label();
            auto expr_munch = expr->munch_bool(muncher, label_true, label_false);
            auto result_temp = muncher.new_temp();

            utils::concat(instr, expr_munch);

            instr.push_back(TAC(
                "label",
                { label_true }
            ));

            instr.push_back(TAC(
                "const",
                { "1" },
                result_temp
            ));

            instr.push_back(TAC(
                "jmp",
                {},
                label_end
            ));

            instr.push_back(TAC(
                "label",
                { label_false }
            ));

            instr.push_back(TAC(
                "const",
                { "0" },
                result_temp
            ));

            instr.push_back(TAC(
                "label",
                { label_end }
            ));

            param_temps.push_back(result_temp);
            param_count++;
        }
        else if (arg_type.is_function()) {
            std::string func_name = dynamic_cast<IdentExpression*>(expr.get())->name;
            std::string code_pointer = muncher.new_temp();
            std::string static_link = muncher.new_temp();

            if (muncher.get_temp(func_name)[0] == '#') {
                instr.push_back(TAC(
                    "const",
                    { func_name },
                    code_pointer
                ));

                instr.push_back(TAC(
                    "const",
                    { "0" },
                    static_link
                ));
            }
            else {
                instr.push_back(TAC(
                    "copy",
                    { muncher.get_temp(func_name) },
                    code_pointer
                ));

                instr.push_back(TAC(
                    "copy",
                    { muncher.get_temp(func_name + "$static_link") },
                    static_link
                ));
            }

            param_temps.push_back(code_pointer);
            param_temps.push_back(static_link);
            param_count += 2;
        }
    }

    std::string code_pointer;
    std::string static_link;

    // not a global function call
    if (muncher.is_defined(name) && muncher.get_temp(name)[0] == '%') {
        code_pointer = muncher.new_temp();
        static_link = muncher.new_temp();

        instr.push_back(TAC(
            "copy",
            { muncher.get_temp(name) },
            code_pointer
        ));

        instr.push_back(TAC(
            "copy",
            { muncher.get_temp(name + "$static_link") },
            static_link
        ));
    }
    // normal function call
    else {
        code_pointer = muncher.new_temp();
        static_link = muncher.new_temp();

        instr.push_back(TAC(
            "const",
            { name },
            code_pointer
        ));

        instr.push_back(TAC(
            "const", 
            { "0" }, 
            static_link
        ));

        instr.push_back(TAC(
            "copy", 
            { static_link }, 
            muncher.new_temp()
        ));
    }

    // add params in reverse order
    reverse(param_temps.begin(), param_temps.end());

    param_count++;

    // static link is last parameter
    instr.push_back(TAC(
        "param",
        { static_link },
        std::to_string(--param_count)
    ));

    // set args only after processing all
    for (auto &temp : param_temps) {
        instr.push_back(TAC(
            "param",
            { temp },
            std::to_string(--param_count)
        ));
    }

    auto res = muncher.new_temp();
    
    instr.push_back(TAC(
        "call",
        { code_pointer, std::to_string(params.size() + 1) },
        res
    ));

    instr.push_back(TAC(
        "jz",
        { res },
        label_false
    ));

    instr.push_back(TAC(
        "jmp",
        {},
        label_true
    ));

    return instr;
}

/*
Statements
*/

[[nodiscard]] std::vector<TAC> ExpressionStatement::munch(MM::MM& muncher) {
    return expr->munch(muncher);
}

[[nodiscard]] std::vector<TAC> VarDecl::munch(MM::MM& muncher) {
    std::vector<TAC> instr;

    for (auto &[name, expr] : var_inits) {
        if (muncher.is_declared(name)) {
            throw std::runtime_error(std::format(
                "Variable '{}' already declared in this scope!", name
            ));
        }
        auto expr_munch = expr->munch(muncher);

        utils::concat(instr, expr_munch);
        muncher.scope().declare(name, type, expr_munch.back().get_result());
    }

    return instr;
}

[[nodiscard]] std::vector<TAC> Assign::munch(MM::MM& muncher) {
    auto temp = muncher.get_temp(name);
    auto var_type = muncher.get_type(name);

    if (var_type.is_int()) {
        auto expr_munch = expr->munch(muncher);
        
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
    return eval->munch(muncher);
}

[[nodiscard]] std::vector<TAC> Jump::munch(MM::MM& muncher) {
    return {TAC(
        "jmp",
        {},
        token.is_type(lexer::BREAK) ? muncher.get_break_point() : muncher.get_continue_point()
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
Lambdas
*/

[[nodiscard]] std::vector<TAC> Lambda::munch(MM::MM& muncher) {
    std::vector<TAC> instr, args_instr, body_instr;

    auto indexed_name = name + muncher.get_function_ind();
    auto lambda_name = muncher.get_function_tree() + indexed_name;
    auto code_pointer = muncher.new_temp();
    instr.push_back(TAC(
        "const",
        { lambda_name },
        code_pointer
    ));

    // get current static link
    auto static_link = muncher.new_temp();
    instr.push_back(TAC(
        "get_fp",
        { },
        static_link
    ));

    // declare function and its static_link
    muncher.scope().declare(name, return_type->to_mm_type(), code_pointer);
    muncher.scope().declare(name + "$static_link", MM::Type::Int(), static_link);

    body_instr.push_back(TAC(
        "proc",
        { },
        lambda_name 
    ));

    muncher.push_scope();
    muncher.push_function_scope();
    muncher.scope().set_function(indexed_name, return_type->to_mm_type());

    body_instr.push_back(TAC(
        "label",
        { muncher.new_label() }
    ));

    for (auto &param : params) {
        auto [name, arg_type] = param.get();
        auto param_temp = muncher.new_temp();

        // immediately move params into temporaries
        if (!arg_type.is_function()) {
            body_instr.push_back(TAC(
                "copy",
                { muncher.new_param_temp() },
                param_temp
            ));
            muncher.scope().declare(name, arg_type, param_temp);
        }
        else {
            body_instr.push_back(TAC(
                "copy",
                { muncher.new_param_temp() },
                param_temp
            ));

            muncher.scope().declare(name, arg_type, param_temp);

            param_temp = muncher.new_temp();
            body_instr.push_back(TAC(
                "copy",
                { muncher.new_param_temp() },
                param_temp
            ));

            muncher.scope().declare(name + "$static_link", MM::Type::Int(), param_temp);
        }
    }

    // mark this copy so it doesnt get removed in CFG
    body_instr.push_back(TAC(
        "copy",
        { muncher.new_param_temp(), "static_link_flag" },
        muncher.new_temp()
    ));

    auto block_munch = block->munch(muncher);
    utils::concat(body_instr, block_munch);

    if (return_type->is_void()) {
        // mark the end of a void lambda
        body_instr.push_back(TAC(
            "ret",
            {}
        ));
    }
    else {
        if (body_instr.back().get_opcode() != "ret") {
            throw std::runtime_error(std::format(
                "Lambda {} has type {}, but has no return!", name, return_type->to_string()
            ));
        }
    }

    muncher.add_lambda(lambda_name, body_instr);

    muncher.pop_function_scope();
    muncher.pop_scope();

#ifdef DEBUG
    std::cout << "Finished munching lambda " << lambda_name << "\n";
#endif

    return instr;
}

/*
Declarations
*/

[[nodiscard]] std::vector<TAC> GlobalVarDecl::munch(MM::MM& muncher) {
    std::vector<TAC> instr;
    for (auto &[name, expr] : var_inits) {
        if (type.is_int()) {
            auto expr_munch = expr->munch(muncher);

            if (expr_munch.size() != 1 || expr_munch.back().get_opcode() != "const") {
                throw std::runtime_error(std::format(
                    "Global variable '{}' can only be initialized with an integer!", name
                ));
            }

            instr.push_back(TAC(
                "const",
                expr_munch.back().get_args(),
                "@" + name
            ));
        }
        else {
            auto expr_munch = expr->munch_bool(muncher, "0", "1");

            instr.push_back(TAC(
                "const",
                { expr_munch.back().get_result() },
                "@" + name
            ));
        }
    }

    return instr;
}

[[nodiscard]] std::vector<TAC> ProcDecl::munch(MM::MM& muncher) {
    std::vector<TAC> instr, args_instr;
    std::vector<std::string> args;

    auto static_link = muncher.new_temp();

    muncher.push_scope();
    muncher.push_function_scope();
    muncher.scope().set_function(name, return_type->to_mm_type());

    for (auto &param : params) {
        auto [name, arg_type] = param.get();
        auto param_temp = muncher.new_temp();

        // immediately move params into temporaries
        if (!arg_type.is_function()) {
            args_instr.push_back(TAC(
                "copy",
                { muncher.new_param_temp() },
                param_temp
            ));
            muncher.scope().declare(name, arg_type, param_temp);
            args.push_back(name);
        }
        else {
            args_instr.push_back(TAC(
                "copy",
                { muncher.new_param_temp() },
                param_temp
            ));

            muncher.scope().declare(name, arg_type, param_temp);
            args.push_back(name);

            param_temp = muncher.new_temp();
            args_instr.push_back(TAC(
                "copy",
                { muncher.new_param_temp() },
                param_temp
            ));

            muncher.scope().declare(name + "$static_link", MM::Type::Int(), param_temp);
            args.push_back(name + "$static_link");
        }
    }

    args.push_back("$static_link");

    // mark this copy so that it doesn't get removed in CFG
    args_instr.push_back(TAC(
        "copy",
        { muncher.new_param_temp(), "static_link_flag" },
        muncher.new_temp()
    ));

    instr.push_back(TAC(
        "proc",
        args,
        name
    ));

    // useful for creating our CFG
    instr.push_back(TAC(
        "label",
        { muncher.new_label() }
    ));

    utils::concat(instr, args_instr);

    auto block_munch = block->munch(muncher);

    utils::concat(instr, block_munch);

    if (return_type->is_void()) {
        // mark the end of a void function
        instr.push_back(TAC(
            "ret",
            {}
        ));
    }
    else {
        if (instr.back().get_opcode() != "ret") {
            throw std::runtime_error(std::format(
                "Function {} has type {}, but has no return!", name, return_type->to_string()
            ));
        }
    }

    muncher.pop_function_scope();
    muncher.pop_scope();

    return instr;
}

[[nodiscard]] std::vector<TAC> Return::munch(MM::MM& muncher) {
    std::vector<TAC> instr;
    std::vector<std::string> args;

    if (!expr) {
        return {TAC(
            "ret",
            {}
        )};
    }
    
    // we have a return expression
    // compare with current function's type
    auto type = muncher.get_curr_function_type();
    if (type.is_int()) {
        instr = expr->munch(muncher);
        args = { instr.back().get_result() };
        instr.push_back(TAC(
            "copy",
            args,
            { muncher.new_temp() }
        ));

        instr.push_back(TAC(
            "ret",
            { instr.back().get_result() }
        ));
    }
    else if (type.is_bool()) {
        auto label_true = muncher.new_label();
        auto label_false = muncher.new_label();

        instr = expr->munch_bool(muncher, label_true, label_false);

        instr.push_back(TAC(
            "label",
            { label_true }
        ));

        instr.push_back(TAC(
            "const",
            { "1" },
            { muncher.new_temp() }
        ));

        instr.push_back(TAC(
            "ret",
            { instr.back().get_result() }
        ));

        instr.push_back(TAC(
            "label",
            { label_false }
        ));

        instr.push_back(TAC(
            "const",
            { "0" },
            { muncher.new_temp() }
        ));

        instr.push_back(TAC(
            "ret",
            { instr.back().get_result() }
        ));
    }
    else {
        // even with a void function we can do
        // return fun();
        // which will just call fun()
        instr = expr->munch(muncher);
    }

    return instr;
}

/*
Program
*/
[[nodiscard]] std::vector<TAC> Program::munch(MM::MM& muncher) {
    std::vector<TAC> instr;
    
    // global scope
    muncher.push_scope();

    for (auto &declaration : declarations)
        declaration->declare(muncher);
    
    for (auto &declaration : declarations) {
        auto decl_munch = declaration->munch(muncher);
        utils::concat(instr, decl_munch);
    }

    std::cout << "Appending lambdas...\n";

    // process lambdas after everything else
    for (auto &[name, lambda_munch] : muncher.get_lambdas())
        utils::concat(instr, lambda_munch);

    muncher.pop_scope();

    return instr;
}


}; // namespace AST