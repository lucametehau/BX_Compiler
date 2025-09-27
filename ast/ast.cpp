#include "ast.h"
#include <cassert>
#include <iostream>

namespace AST {

/*
Expressions
*/

[[nodiscard]] std::vector<TAC> NumberExpression::munch(MM::MM& muncher) const {
    TAC tac;
    tac["opcode"] = {"const"};
    tac["args"] = { std::to_string(value) };
    tac["result"] = { muncher.new_temp() };
    return {tac};
}

[[nodiscard]] std::vector<TAC> IdentExpression::munch(MM::MM& muncher) const {
    TAC tac;
    tac["opcode"] = {"copy"};
    tac["args"] = { muncher.scope().get_temp(name) };
    tac["result"] = { muncher.new_temp() };
    return {tac};
}

[[nodiscard]] std::vector<TAC> UniOpExpression::munch(MM::MM& muncher) const {
    TAC tac;
    std::vector<TAC> expr_munch = expr->munch(muncher);
    tac["opcode"] = { Lexer::op_code.find(token.get_text())->second };
    tac["args"] = { expr_munch.back()["result"][0] }; // expression result stored in this
    tac["result"] = { muncher.new_temp() };
    expr_munch.push_back(tac);
    return expr_munch;
}

[[nodiscard]] std::vector<TAC> BinOpExpression::munch(MM::MM& muncher) const {
    TAC tac;
    std::vector<TAC> left_munch = left->munch(muncher);
    std::vector<TAC> right_munch = right->munch(muncher);
    tac["opcode"] = { Lexer::op_code.find(token.get_text())->second };
    tac["args"] = { left_munch.back()["result"][0], right_munch.back()["result"][0] }; // expression result stored in this
    tac["result"] = { muncher.new_temp() };
    for (auto &t : right_munch)
        left_munch.push_back(t);
    left_munch.push_back(tac);
    return left_munch;
}

/*
Bool Expression
*/
[[nodiscard]] std::vector<TAC> BoolExpression::munch(MM::MM& muncher, std::string label_true, std::string label_false) {
    return {};
}

/*
Statements
*/

[[nodiscard]] std::vector<TAC> VarDecl::munch(MM::MM& muncher) const {
    std::vector<TAC> expr_munch = expr->munch(muncher);
    if (muncher.scope().is_declared(name)) {
        throw std::runtime_error("Variable " + name + " already declared\n");
    }

    assert (!expr_munch.back()["result"].empty());
    muncher.scope().declare(name, expr_munch.back()["result"][0]);
    return expr_munch;
}

[[nodiscard]] std::vector<TAC> Assign::munch(MM::MM& muncher) const {
    TAC tac;
    std::vector<TAC> expr_munch = expr->munch(muncher);
    tac["opcode"] = { "copy" };
    tac["args"] = { expr_munch.back()["result"][0] };
    tac["result"] = { muncher.scope().get_temp(name) };
    expr_munch.push_back(tac);
    return expr_munch;
}

[[nodiscard]] std::vector<TAC> Print::munch(MM::MM& muncher) const {
    TAC tac;
    std::vector<TAC> expr_munch = expr->munch(muncher);
    tac["opcode"] = { "print" };
    tac["args"] = { expr_munch.back()["result"][0] };
    tac["result"] = { };
    expr_munch.push_back(tac);
    return expr_munch;
}

/*
Block
*/
[[nodiscard]] std::vector<TAC> Block::munch(MM::MM& muncher) const {
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
[[nodiscard]] std::vector<TAC> IfElse::munch(MM::MM& muncher) const {
    auto label_then = muncher.new_label(), label_end = muncher.new_label();
    TAC tac;
    if (!else_branch.has_value()) {
        auto expr_munch = expr->munch(muncher, label_then, label_end);
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
    auto expr_munch = expr->munch(muncher, label_then, label_else);
    auto then_munch = then_branch->munch(muncher);
    auto else_munch = std::visit([&](auto& p) {
        return p ? p->munch(muncher) : std::vector<TAC>{};
    }, else_branch.value());

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

    // else
    expr_munch.push_back(tac);
    tac["opcode"] = { "label" };
    tac["args"] = { label_else };
    tac["result"] = {};
    expr_munch.push_back(tac);
    expr_munch.push_back(tac);
    for (auto &t : else_munch)
        expr_munch.push_back(t);
    tac["opcode"] = { "label" };
    tac["args"] = { label_end };
    tac["result"] = {};

    return expr_munch;
}

/*
Program
*/
[[nodiscard]] std::vector<TAC> Program::munch(MM::MM& muncher) const {
    return block->munch(muncher);
}


}; // namespace AST