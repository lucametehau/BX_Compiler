#pragma once
#include "../mm/tac.h"
#include <fstream>
#include <set>
#include <map>
#include <functional>

namespace ASM {

class Assembler {
private:
    std::size_t stack_size;
    std::vector<TAC> instr;

public:
    Assembler(std::vector<TAC>& _instr);

    void assemble(std::ofstream& os);

private:
    std::string stack_register(std::string& temp) {
        assert(temp[0] == '%');
        std::cout << temp.substr(1) << "\n";
        return "-" + std::to_string(8 * std::stoi(temp.substr(1))) + "(%rbp)";
    }

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
        os << "\tmovq " << a << ", %r8\n";
        os << "\tmovq " << b << ", %rcx\n";
        os << "\tsalq %cl, %r8\n";
        os << "\tmovq %r8, " << res << "\n";
    }},
    {"shr", [](std::string a, std::string b, std::string res, std::ofstream& os) {
        os << "\tmovq " << a << ", %r8\n";
        os << "\tmovq " << b << ", %rcx\n";
        os << "\tsarq %cl, %r8\n";
        os << "\tmovq %r8, " << res << "\n";
    }}
};

};