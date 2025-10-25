#pragma once
#include <map>
#include <string>
#include <cassert>
#include <vector>
#include <iostream>
#include <optional>
#include <format>
#include "tac.h"
#include "../lexer/token.h"

namespace MM {

enum class Type {
    INT,
    BOOL,
    VOID,
};

inline std::map<Type, std::string> type_text = {
    {Type::INT, "int"}, {Type::BOOL, "bool"}, {Type::VOID, "void"}
};

inline std::map<Lexer::Type, Type> lexer_to_mm_type = {
    {Lexer::INT, Type::INT}, {Lexer::BOOL, Type::BOOL}, {Lexer::VOID, Type::VOID}
};

struct Symbol {
    std::string name;
    Type type;
    std::string temp; // temporary
    bool is_function; // used to identify function symbols
};

class Scope {
private:
    std::map<std::string, Symbol> temp_map;
    std::optional<Type> function_type;
    std::map<std::pair<std::string, std::size_t>, Type> arg_type;

public:
    void declare(std::string name, Type type, std::string temp, bool is_function = false) {
        temp_map[name] = Symbol{name, type, temp, is_function};
    }

    // sets the type of the function in which the scope is
    void set_function_type(Type type) {
        function_type = type;
    }

    void set_arg_type(std::string name, std::size_t arg_id, Type type) {
        arg_type[{name, arg_id}] = type;
    }

    [[nodiscard]] std::string get_temp(const std::string& name) const {
        auto it = temp_map.find(name);
        assert (it != temp_map.end());
        return it->second.temp;
    }

    [[nodiscard]] Type get_type(const std::string& name) const {
        auto it = temp_map.find(name);
        assert (it != temp_map.end());
        return it->second.type;
    }

    [[nodiscard]] std::optional<Type> get_current_function_type() const {
        return function_type;
    }

    [[nodiscard]] Type get_function_arg_type(std::string name, std::size_t arg_id) const {
        auto it = arg_type.find({name, arg_id});
        if (it == arg_type.end()) {
            throw std::runtime_error(std::format(
                "Function '{}' called but not defined!", name
            ));
        }
        return it->second;
    }

    [[nodiscard]] bool is_declared(const std::string& name) const {
        auto it = temp_map.find(name);
        return it != temp_map.end();
    }
};

class MM {
    int temp_ind, label_ind, param_temp_ind;
    std::vector<Scope> scopes;
    std::vector<std::string> break_point_stack;
    std::vector<std::string> continue_point_stack;

    std::vector<std::pair<std::size_t, std::size_t>> procs;
    std::vector<TAC> globals;

public:
    MM() : temp_ind(0), label_ind(0), break_point_stack({}), continue_point_stack({}) {}

    void push_scope() { scopes.emplace_back(); }

    void pop_scope() { scopes.pop_back(); }

    void push_break_point(std::string label) { break_point_stack.push_back(label); }

    void push_continue_point(std::string label) { continue_point_stack.push_back(label); }

    void pop_break_point() { break_point_stack.pop_back(); }

    void pop_continue_point() { continue_point_stack.pop_back(); }

    void init_function_scope() { param_temp_ind = 0; }

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

    [[nodiscard]] std::string new_temp()  {
        return "%" + std::to_string(temp_ind++);
    }

    [[nodiscard]] std::string new_label()  {
        return "%.L" + std::to_string(label_ind++);
    }

    [[nodiscard]] std::string new_param_temp() {
        return "%p" + std::to_string(param_temp_ind++);
    }

    // gets the used temporary of the variable with name <name>
    [[nodiscard]] std::string get_temp(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); it++) {
            if (it->is_declared(name))
                return it->get_temp(name);
        }
        throw std::runtime_error("Variable " + name + " undeclared!");
    }

    // gets type of variable with name <name>
    [[nodiscard]] Type get_type(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); it++) {
            if (it->is_declared(name))
                return it->get_type(name);
        }
        throw std::runtime_error("Variable " + name + " undeclared!");
    }

    // gets the type of the function in which we currently are
    [[nodiscard]] Type get_curr_function_type() const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); it++) {
            if (it->get_current_function_type().has_value())
                return it->get_current_function_type().value();
        }
        throw std::runtime_error("Not in a function's scope!");
    }

    // gets the type of the function in which we currently are
    [[nodiscard]] Type get_function_arg_type(std::string name, std::size_t arg_id) const {
        return scopes.begin()->get_function_arg_type(name, arg_id);
    }

    [[nodiscard]] bool is_declared(const std::string& name) const {
        return !scopes.empty() && scopes.rbegin()->is_declared(name);
    }

    void process(std::vector<TAC>& instructions) {
        std::size_t ind = 0;

        std::cout << "######################################\n";
        for (auto & tac : instructions)
            std::cout << tac << "\n";
        std::cout << "######################################\n";

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
            out << tab << "\"proc\": \"@" << instructions[start].get_result() << "\",\n";
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