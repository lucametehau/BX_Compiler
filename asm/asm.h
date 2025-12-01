#pragma once
#include "../mm/tac.h"
#include "../mm/mm.h"
#include <fstream>
#include <set>
#include <map>
#include <functional>

namespace assembly {

inline const std::array<std::string, 6> arg_registers = {
    "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"
};

class Assembler {
private:
    MM::MM muncher;
    int args_on_stack, static_link_arg;
    std::size_t stack_offset, stack_size;
    
    // current function name
    std::string curr_func_name;

    // last register used for walking the static link chain
    std::string last_capture_register;

    std::vector<TAC> instr;

    std::map<std::size_t, std::string> param_register;

    // function in which temporary is defined
    std::map<std::string, std::string> func_of_temp;

    // keeps the min and max temporaries which are strictly from a function
    std::map<std::string, std::pair<std::size_t, std::size_t>> bounds;

    // keeps the assembly name given to a function
    std::map<std::string, std::string> asm_name;

    std::ofstream& os;

public:
    Assembler(MM::MM& muncher, std::vector<TAC>& _instr, std::ofstream& os);

    void assemble();

private:
    void set_param_register(std::size_t id, std::string reg) {
        param_register[id] = reg;
    }

    std::string compute_offset(std::string temp, std::size_t offset, std::string offset_register = "%rbp") {
        return "-" + std::to_string(8 * (std::stoi(temp.substr(1)) - offset + 2)) + "(" + offset_register + ")";
    }

    std::string stack_register(std::string temp) {
        // global variable
        if (temp[0] == '@')
            return temp.substr(1) + "(%rip)";

        assert(temp[0] == '%');

        // parameter
        if (temp[1] == 'p') {
            auto id = std::stoi(temp.substr(2));
            if (id < 6)
                return arg_registers[id];
            return std::to_string(8 * (id - 6 + 2)) + "(%rbp)";
        }

        auto origin_func = func_of_temp[temp];
        if (origin_func == curr_func_name)
            return compute_offset(temp, stack_offset);
        else {
            int delta = 0;
            auto curr = curr_func_name;

            auto get_parent_function = [&](std::string &func_name) {
                for (auto i = func_name.size() - 1; i >= 0; i--) {
                    if (func_name[i] == ':')
                        return func_name.substr(0, i-1);
                }
                return std::string();
            };

            while (curr != origin_func) {
                curr = get_parent_function(curr);
                delta++;
            }

            // special case:
            // when doing a binary operation with captures and removing dead-copies
            // we actually remove capture copies
            // since we were using the same %r11 to iterate on the static link chain
            // we actually overwrite stuff. a simple solution is to alternate to %r12
            last_capture_register = last_capture_register != "%r11" ? "%r11" : "%r12";

            os << "\n\t# Capture access, delta = " << delta << " from " << curr_func_name << " to " << origin_func << "\n";
            os << "\tmovq -8(%rbp), " << last_capture_register << "\n";
            for (int i = 1; i < delta; i++)
                os << "\tmovq -8(" << last_capture_register << "), " << last_capture_register << "\n";

            return compute_offset(temp, bounds[curr].first, last_capture_register);
        }
    }

    void process_proc(std::size_t start, std::size_t finish);

    void assemble_proc(std::size_t start, std::size_t finish);

    void assemble_instr(TAC& tac);
};

static const std::set<std::string> jumps = {
    "jz", "jnz", "jl", "jle", "jg", "jge"
};

static const std::map<std::string, std::string> uniops = {
    {"neg", "negq"}, {"not", "notq"}
};

static const std::map<std::string, std::string> normal_binops = {
    {"add", "addq"}, {"sub", "subq"}, {"and", "andq"}, {"or", "orq"}, {"xor", "xorq"}
};

static const std::map<std::string, std::function<void(std::string, std::string, std::string, std::ofstream&)>> special_binops = {
    {"mul", [](std::string a, std::string b, std::string res, std::ofstream& os) {
        os << "\tmovq " << a << ", %rax\n";
        os << "\timulq " << b << "\n";
        os << "\tmovq %rax, " << res << "\n";
    }},
    {"div", [](std::string a, std::string b, std::string res, std::ofstream& os) {
        os << "\tmovq " << a << ", %rax\n";
        os << "\tcqto\n";
        os << "\tidivq " << b << "\n";
        os << "\tmovq %rax, " << res << "\n";
    }},
    {"mod", [](std::string a, std::string b, std::string res, std::ofstream& os) {
        os << "\tmovq " << a << ", %rax\n";
        os << "\tcqto\n";
        os << "\tidivq " << b << "\n";
        os << "\tmovq %rdx, " << res << "\n";
    }},
    {"shl", [](std::string a, std::string b, std::string res, std::ofstream& os) {
        os << "\tmovq " << a << ", %r10\n";
        os << "\tmovq " << b << ", %rcx\n";
        os << "\tsalq %cl, %r10\n";
        os << "\tmovq %r10, " << res << "\n";
    }},
    {"shr", [](std::string a, std::string b, std::string res, std::ofstream& os) {
        os << "\tmovq " << a << ", %r10\n";
        os << "\tmovq " << b << ", %rcx\n";
        os << "\tsarq %cl, %r10\n";
        os << "\tmovq %r10, " << res << "\n";
    }}
};

};