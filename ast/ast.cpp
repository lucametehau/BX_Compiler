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
Special Rules
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

[[nodiscard]] std::vector<TAC> Program::munch(MM::MM& muncher) const {
    return block->munch(muncher);
}


}; // namespace AST