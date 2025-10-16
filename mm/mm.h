#pragma once
#include <map>
#include <string>
#include <cassert>
#include <vector>
#include <iostream>
#include "tac.h"
#include "../lexer/token.h"

namespace MM {

enum class Type {
    INT,
    BOOL,
    NONE
};

inline std::map<Type, std::string> type_text = {
    {Type::INT, "int"}, {Type::BOOL, "bool"}, {Type::NONE, ""}
};

inline std::map<Lexer::Type, Type> lexer_to_mm_type = {
    {Lexer::INT, Type::INT}, {Lexer::BOOL, Type::BOOL}, {Lexer::VOID, Type::NONE}
};

struct Symbol {
    std::string name;
    Type type;
    std::string temp; // temporary
};

class Scope {
private:
    std::map<std::string, Symbol> temp_map;

public:
    void declare(std::string name, Type type, std::string temp) {
        temp_map[name] = Symbol{name, type, temp};
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

    [[nodiscard]] bool is_declared(const std::string& name) const {
        auto it = temp_map.find(name);
        return it != temp_map.end();
    }
};

class MM {
    int temp_ind, label_ind;
    std::vector<Scope> scopes;
    std::vector<std::string> break_point_stack;
    std::vector<std::string> continue_point_stack;

public:
    MM() : temp_ind(0), label_ind(0), break_point_stack({}), continue_point_stack({}) {}

    void push_scope() { scopes.emplace_back(); }

    void pop_scope() { scopes.pop_back(); }

    void push_break_point(std::string& label) { break_point_stack.push_back(label); }

    void push_continue_point(std::string& label) { continue_point_stack.push_back(label); }

    void pop_break_point() { break_point_stack.pop_back(); }

    void pop_continue_point() { continue_point_stack.pop_back(); }

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

    [[nodiscard]] std::string new_temp()  {
        return "%" + std::to_string(temp_ind++);
    }

    [[nodiscard]] std::string new_label()  {
        return "%.L" + std::to_string(label_ind++);
    }

    [[nodiscard]] std::string new_param_temp() {
        return "%p" + std::to_string(temp_ind++);
    }

    [[nodiscard]] std::string get_temp(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); it++) {
            if (it->is_declared(name))
                return it->get_temp(name);
        }
        throw std::runtime_error("Variable " + name + " undeclared!");
    }

    [[nodiscard]] Type get_type(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); it++) {
            if (it->is_declared(name))
                return it->get_type(name);
        }
        throw std::runtime_error("Variable " + name + " undeclared!");
    }

    [[nodiscard]] bool is_declared(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); it++) {
            if (it->is_declared(name))
                return true;
        }
        return false;
    }

    void jsonify(std::string filename, std::vector<TAC>& instructions) {
        std::ofstream out(filename);
        out << "[\n";
        std::size_t ind = 0;

        while (ind < instructions.size()) {
            if (instructions[ind].get_opcode() != "proc") {
                out << std::string(2, ' ') << instructions[ind++] << ",\n";
                continue;
            }

            auto j = ind + 1;
            while (j < instructions.size() && instructions[j].get_opcode() != "proc")
                j++;
            j--;
            
            while (j && instructions[j].get_opcode() != "ret")
                j--;

            // std::cout << ind << " " << j << " " << instructions.size() << "\n";
            
            // procedure between [ind, j]
            auto tab = std::string(2, ' ');
            out << tab << "{\n";
            tab += "  ";
            out << tab << "\"proc\": \"@" << instructions[ind].get_result() << "\",\n";
            out << tab << "\"body\": [\n";
            tab += "  ";
            for (std::size_t i = ind + 1; i <= j; i++) {
                out << tab << instructions[i];
                if (i != j) out << ",";
                out << "\n";
            }
            out << "    ]\n";
            out << "  }";

            ind = j + 1;
            if (ind != instructions.size())
                out << ",\n";
            else
                out << "\n";
        }
        out << "]\n";
    }
};

}; // namespace MM