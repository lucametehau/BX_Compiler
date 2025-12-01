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
    std::vector<TAC> instr;
    std::map<std::size_t, std::string> param_register;

public:
    Assembler(MM::MM& muncher, std::vector<TAC>& _instr);

    void assemble(std::ofstream& os);

private:
    void set_param_register(std::size_t id, std::string reg) {
        param_register[id] = reg;
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

        // normal temporary
        return "-" + std::to_string(8 * (std::stoi(temp.substr(1)) - stack_offset + 1)) + "(%rbp)";
    }

    void assemble_proc(std::ofstream& os, std::size_t start, std::size_t finish);

    void assemble_instr(std::ofstream& os, TAC& tac);
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