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
#include "../lexer/token.h"

namespace MM {

struct Type {
    enum Kind {
        INT,
        BOOL,
        VOID,
        FUNCTION
    } kind;

    std::vector<Type> param_types;
    std::unique_ptr<Type> return_type; // todo, make unique_ptr

    Type() = default;
    Type(Kind k) : kind(k) {}

    Type(const Type& other) : kind(other.kind), param_types(other.param_types) {
        if (other.return_type)
            return_type = std::make_unique<Type>(*other.return_type);
    }

    Type& operator=(const Type& other) {
        if (this != &other) {
            kind = other.kind;
            param_types = other.param_types;
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

    static Type Function(std::vector<Type> params, Type ret) {
        Type t(FUNCTION);
        t.param_types = std::move(params);
        t.return_type = std::make_unique<Type>(ret);
        return t;
    }

    bool operator == (const Type &other) const {
        if (kind != other.kind)
            return false;
        
        return kind != FUNCTION || 
               (param_types == other.param_types && 
               *return_type == *other.return_type);
    }

    constexpr bool is_int() const { return kind == INT; }
    constexpr bool is_bool() const { return kind == BOOL; }
    constexpr bool is_void() const { return kind == VOID; }

    constexpr bool is_first_order() const {
        return kind == INT || kind == BOOL || kind == VOID;
    }

    constexpr bool is_function() const { return kind == FUNCTION; }

    const Type get_param_type(std::size_t ind) const {
        assert (kind == FUNCTION);
        assert (ind < param_types.size());
        return param_types[ind];
    }

    const Type get_return_type() const {
        assert (kind == FUNCTION);
        assert (return_type);
        return *return_type;
    }

    std::string to_string() const {
        switch (kind) {
            case INT: return "int";
            case BOOL: return "bool";
            case VOID: return "void";
            case FUNCTION: {
                std::string s = "function(";
                for (size_t i = 0; i < param_types.size(); i++) {
                    s += param_types[i].to_string();
                    if (i + 1 < param_types.size()) s += ", ";
                }
                s += ") -> " + return_type->to_string();
                return s;
            }
        }
        return "<?>";
    }
};

inline std::map<lexer::Type, Type> lexer_to_mm_type = {
    {lexer::INT, Type::Int()}, {lexer::BOOL, Type::Bool()}, {lexer::VOID, Type::Void()}
};

struct Symbol {
    std::string name;
    Type type;
    std::string temp; // temporary
};

class Scope {
private:
    std::map<std::string, Symbol> temp_map;
    std::optional<Type> function_type;

public:
    void declare(std::string name, Type type, std::string temp) {
        temp_map[name] = Symbol{name, type, temp};
    }

    // sets the type of the function in which the scope is
    void set_function_type(Type type) {
        function_type = type;
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

    [[nodiscard]] bool is_declared(const std::string& name) const {
        return !scopes.empty() && scopes.rbegin()->is_declared(name);
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