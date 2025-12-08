#pragma once
#include <map>
#include <string>
#include <cassert>
#include <vector>
#include <iostream>
#include <optional>
#include <format>
#include <memory>
#include "tac.h"
#include "scope.h"

namespace MM {

class MM {
    int temp_ind, label_ind, param_temp_ind, function_ind;
    std::vector<Scope> scopes;
    std::vector<std::string> break_point_stack;
    std::vector<std::string> continue_point_stack;
    std::vector<int> function_ind_stack;
    std::vector<Temporary> static_links;

    std::vector<std::pair<std::size_t, std::size_t>> procs;
    std::vector<TAC> globals;

    std::vector<std::pair<std::string, std::vector<TAC>>> lambdas;
    std::map<Temporary, std::string> func_of_temp;

public:
    MM() : temp_ind(0), label_ind(0), function_ind(0), break_point_stack({}), continue_point_stack({}) {}

    void push_scope() { scopes.emplace_back(); }

    void pop_scope() { scopes.pop_back(); }

    void push_break_point(std::string label) { break_point_stack.push_back(label); }

    void push_continue_point(std::string label) { continue_point_stack.push_back(label); }

    void pop_break_point() { break_point_stack.pop_back(); }

    void pop_continue_point() { continue_point_stack.pop_back(); }

    void push_function_scope() {
        function_ind_stack.push_back(++function_ind);
        param_temp_ind = 0; 
    }

    void pop_function_scope() {
        function_ind_stack.pop_back();
    }

    void add_lambda(std::string name, std::vector<TAC> &lambda_instr) {
        lambdas.push_back({name, lambda_instr});
    }

    [[nodiscard]] std::string get_break_point() const {
        assert(!break_point_stack.empty());
        return break_point_stack.back();
    }

    [[nodiscard]] std::string get_continue_point() const {
        assert(!continue_point_stack.empty());
        return continue_point_stack.back();
    }

    [[nodiscard]] Scope& scope() { 
        assert(!scopes.empty());
        return scopes.back();
    }

    [[nodiscard]] std::vector<TAC> get_globals() {
        return globals;
    }

    [[nodiscard]] std::vector<std::pair<std::size_t, std::size_t>> procs_indexes() {
        return procs;
    }

    [[nodiscard]] std::vector<std::pair<std::string, std::vector<TAC>>> get_lambdas() {
        return lambdas;
    }

    [[nodiscard]] std::map<Temporary, std::string> &get_func_of_temps() {
        return func_of_temp;
    }

    [[nodiscard]] Temporary new_temp()  {
        auto temp = "%" + std::to_string(temp_ind++);
        func_of_temp[temp] = get_function_tree(); // declare temporary's function
        func_of_temp[temp] = func_of_temp[temp].substr(0, func_of_temp[temp].size() - 2); // without the ::
        return temp;
    }

    [[nodiscard]] Temporary new_label()  {
        return "%.L" + std::to_string(label_ind++);
    }

    [[nodiscard]] Temporary new_param_temp() {
        return "%p" + std::to_string(param_temp_ind++);
    }

    // + 1 since its called right before push_function_scope
    [[nodiscard]] std::string get_function_ind() {
        return std::to_string(function_ind + 1);
    }

    // gets the used temporary of the variable with name <name>
    [[nodiscard]] Temporary get_temp(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); it++) {
            if (it->is_declared(name))
                return it->get_temp(name);
        }
        throw std::runtime_error("Variable " + name + " undeclared, unable to retrieve temporary!");
    }

    // gets type of variable with name <name>
    [[nodiscard]] Type get_type(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); it++) {
            if (it->is_declared(name))
                return it->get_type(name);
        }
        throw std::runtime_error("Variable " + name + " undeclared, unable to retrieve type!");
    }

    // gets the type of the function in which we currently are
    [[nodiscard]] Type get_curr_function_type() const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); it++) {
            if (it->get_current_function_type().has_value())
                return it->get_current_function_type().value();
        }
        throw std::runtime_error("Not in a function's scope!");
    }

    // gets the function imbrication tree as string
    [[nodiscard]] std::string get_function_tree() const {
        std::string name = "";
        for (auto it = next(scopes.begin()); it != scopes.end(); it++) {
            if (it->get_current_function_name().has_value())
                name += it->get_current_function_name().value() + "::";
        }
        return name;
    }

    // checks if variable is declared in most recent scope
    [[nodiscard]] bool is_declared(const std::string& name) const {
        return !scopes.empty() && scopes.rbegin()->is_declared(name);
    }

    // checks if variable is declared anywheree
    [[nodiscard]] bool is_defined(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); it++) {
            if (it->is_declared(name))
                return true;
        }
        return false;
    }

    void process(std::vector<TAC>& instructions) {
        std::size_t ind = 0;

        globals.clear();
        procs.clear();

        while (ind < instructions.size()) {
            if (instructions[ind].get_opcode() != "proc") {
                globals.push_back(instructions[ind++]);
                continue;
            }

            auto j = ind + 1;
            while (j < instructions.size() && instructions[j].get_opcode() != "proc")
                j++;
            j--;
            
            while (j && instructions[j].get_opcode() != "ret")
                j--;

            procs.push_back(std::make_pair(ind, j));

            ind = j + 1;
        }
    }

    void jsonify(std::string filename, std::vector<TAC>& instructions) {
        process(instructions);

        std::ofstream out(filename);
        out << "[\n";
        
        for (auto &global : get_globals())
            out << std::string(2, ' ') << global << ",\n";

        for (auto &[start, finish] : procs_indexes()) {
            auto tab = std::string(2, ' ');
            out << tab << "{\n";
            tab += "  ";
            out << tab << "\"proc\": \"" << instructions[start].get_result() << "\",\n";
            out << tab << "\"body\": [\n";
            tab += "  ";
            for (std::size_t i = start + 1; i <= finish; i++) {
                out << tab << instructions[i];
                if (i != finish) out << ",";
                out << "\n";
            }
            out << "    ]\n";
            out << "  }";

            if (finish + 1 != instructions.size())
                out << ",\n";
            else
                out << "\n";
        }

        out << "]\n";
    }
};

}; // namespace MM