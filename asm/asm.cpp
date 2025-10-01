#include "asm.h"

namespace ASM {

Assembler::Assembler(std::vector<TAC>& _instr) : instr(std::move(_instr)) {
    stack_size = 0;
    for (auto &t : instr) {
        auto args = t.get_args();
        
        for (auto &temp : args) {
            if (temp[0] == '%' && temp[1] != '.')
                stack_size = std::max(stack_size, static_cast<std::size_t>(std::stoi(temp.substr(1))));
        }

        if (t.has_result()) {
            auto result = t.get_result();
            if (result[0] == '%' && result[1] != '.')
                stack_size = std::max(stack_size, static_cast<std::size_t>(std::stoi(result.substr(1))));
        }
    }

    stack_size = (stack_size + 1) / 2 * 2;
}

void Assembler::assemble(std::ofstream& os) {
    os << "\t.text\n";
    os << "\t.globl main\n";
    os << "main:\n";
    os << "\tpushq %rbp\n";
    os << "\tmovq %rsp, %rbp\n";
    os << "\tsubq $" << 8 * stack_size << ", %rsp\n";

    for (auto &t : instr) {
        assemble_instr(os, t);
    }

    os << "\tmovq %rbp, %rsp\n";
    os << "\tpopq %rbp\n";
    os << "\tmovq $0, %rax\n";
    os << "\tretq\n";
}

void Assembler::assemble_instr(std::ofstream& os, TAC& tac) {
    auto op = tac.get_opcode();
    auto args = tac.get_args();

    if (op == "label") {
        assert(args.size() == 1);
        os << args[0].substr(1) << ":\n";
    }
    else if (op == "const") {
        assert(args.size() == 1 && tac.has_result());
        os << "\tmovq $" << args[0] << ", " << stack_register(tac.get_result()) << "\n";
    }
    else if (op == "copy") {
        assert(args.size() == 1 && tac.has_result());
        os << "\tmovq " << stack_register(args[0]) << ", %r8\n";
        os << "\tmovq %r8, " << stack_register(tac.get_result()) << "\n";
    }
    else if (op == "print") {
        assert(args.size() == 1);
#ifdef _WIN32
        os << "\tmovq " << stack_register(args[0]) << ", %rcx\n";
        os << "\tcallq bx_print_int\n";
#else
        os << "\tmovq " << stack_register(args[0]) << ", %rdi\n";
        os << "\tcallq bx_print_int\n";
#endif
    }
    else if (op == "jmp") {
        assert(args.size() == 1 || (args.empty() && tac.has_result()));
        auto label = args.empty() ? tac.get_result() : args[0];
        os << "\tjmp " << label.substr(1) << "\n";
    }
    else if (jumps.count(op)) {
        assert(args.size() == 1 && tac.has_result());
        os << "\tcmpq $0, " << stack_register(args[0]) << "\n";
        os << "\t" << op << " " << tac.get_result().substr(1) << "\n";
    }
    else if (auto it = uniops.find(op); it != uniops.end()) {
        assert(args.size() == 1 && tac.has_result());
        os << "\tmovq " << stack_register(args[0]) << ", %r8\n";
        os << "\t" << it->second << " %r8\n";
        os << "\tmovq %r8, " << stack_register(tac.get_result()) << "\n";
    }
    else if (auto it = normal_binops.find(op); it != normal_binops.end()) {
        assert(args.size() == 2 && tac.has_result());
        os << "\tmovq " << stack_register(args[0]) << ", %r8\n";
        os << "\t" << it->second << " " << stack_register(args[1]) << ", %r8\n";
        os << "\tmovq %r8, " << stack_register(tac.get_result()) << "\n";
    }
    else if (auto it = special_binops.find(op); it != special_binops.end()) {
        assert(args.size() == 2 && tac.has_result());
        it->second(stack_register(args[0]), stack_register(args[1]), stack_register(tac.get_result()), os);
    }
    else {
        throw std::runtime_error("Unrecognized operator " + op);
    }
}

}; // namespace ASM