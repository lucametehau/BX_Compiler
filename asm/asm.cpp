#include "asm.h"
#include <iomanip>

namespace ASM {

Assembler::Assembler(MM::MM& muncher, std::vector<TAC>& _instr) : muncher(muncher), args_on_stack(0), instr(_instr) {}

void Assembler::assemble(std::ofstream& os) {
    // global variables
    for (auto &tac : muncher.get_globals()) {
        auto name = tac.get_result().substr(1);
        os << "\t.globl " << name << "\n";
        os << "\t.data\n";
        os << name << ":" << std::setw(8) << ".quad " << std::stoi(tac.get_arg()) << "\n";
    }

    // procedures
    for (auto &[start, finish] : muncher.procs_indexes()) {
        auto name = instr[start].get_result();
        os << "\n";
        os << "\t.globl " << name << "\n";
        os << "\t.text\n";
        os << name << ":\n";
        assemble_proc(os, start, finish);
    }

    os.close();
}

void Assembler::assemble_proc(std::ofstream& os, std::size_t start, std::size_t finish) {
    // compute needed registers
    stack_size = 0;

    std::size_t mn = 1e9, mx = 0;
    for (auto i = start + 1; i <= finish; i++) {
        auto t = instr[i];
        auto args = t.get_args();

#ifdef DEBUG
        std::cout << t << "\n";
#endif
        
        for (auto &temp : args) {
            if (temp[0] == '%' && temp[1] != '.' && temp[1] != 'p') {
                auto num = static_cast<std::size_t>(std::stoi(temp.substr(1)));
                mn = std::min(mn, num);
                mx = std::max(mx, num);
            }
        }

        if (t.has_result()) {
            auto result = t.get_result();
            if (result[0] == '%' && result[1] != '.') {
                auto num = static_cast<std::size_t>(std::stoi(result.substr(1)));
                mn = std::min(mn, num);
                mx = std::max(mx, num);
            }
        }
    }

    if (mx == 0) mn = mx = 0;

    stack_size = mx - mn + 1 + instr[start].get_args().size();
    stack_size = (stack_size + 1) / 2 * 2;
    stack_offset = mn;

#ifdef DEBUG
    std::cout << stack_offset << "->" << mx << " ";
#endif

    os << "\tpushq %rbp\n";
    os << "\tmovq %rsp, %rbp\n";
    os << "\tsubq $" << 8 * stack_size << ", %rsp\n";

#ifdef DEBUG
    std::cout << stack_size << "\n";
#endif

    for (auto i = start + 1; i <= finish; i++) {
        assemble_instr(os, instr[i]);
    }
}

void Assembler::assemble_instr(std::ofstream& os, TAC& tac) {
    auto op = tac.get_opcode();
    auto args = tac.get_args();

#ifdef DEBUG
    std::cout << tac << "\n";
#endif

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
        os << "\tmovq " << stack_register(args[0]) << ", %r10\n";
        os << "\tmovq %r10, " << stack_register(tac.get_result()) << "\n";
    }
    else if (op == "call") {
        os << "\tcallq " << args[0].substr(1) << "\n";

        if (args_on_stack)
            os << "\taddq $" << 8 * ((args_on_stack + 1) / 2 * 2) << ", %rsp\n";
        args_on_stack = 0;

        if (tac.has_result())
            os << "\tmovq %rax, " << stack_register(tac.get_result()) << "\n";
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
        os << "\tmovq " << stack_register(args[0]) << ", %r10\n";
        os << "\t" << it->second << " %r10\n";
        os << "\tmovq %r10, " << stack_register(tac.get_result()) << "\n";
    }
    else if (auto it = normal_binops.find(op); it != normal_binops.end()) {
        assert(args.size() == 2 && tac.has_result());
        os << "\tmovq " << stack_register(args[0]) << ", %r10\n";
        os << "\t" << it->second << " " << stack_register(args[1]) << ", %r10\n";
        os << "\tmovq %r10, " << stack_register(tac.get_result()) << "\n";
    }
    else if (auto it = special_binops.find(op); it != special_binops.end()) {
        assert(args.size() == 2 && tac.has_result());
        it->second(stack_register(args[0]), stack_register(args[1]), stack_register(tac.get_result()), os);
    }
    else if (op == "ret") {
        if (!args.empty())
            os << "\tmovq " << stack_register(args[0]) << ", %rax\n";
        else
            os << "\tmovq $0, %rax\n";
        os << "\tmovq %rbp, %rsp\n";
        os << "\tpopq %rbp\n";
        os << "\tretq\n";
    }
    else if (op == "param") {
        auto id = std::stoi(tac.get_result());

        if (id <= 6)
            os << "\tmovq " << stack_register(args[0]) << ", " << arg_registers[id - 1] << "\n";
        else {
            os << "\tpushq " << stack_register(args[0]) << "\n";
            args_on_stack++;
        }
    }
    else {
        throw std::runtime_error("Unrecognized operator " + op);
    }
}

}; // namespace ASM