#include "asm.h"
#include <iomanip>

namespace assembly {

Assembler::Assembler(MM::MM& muncher, std::vector<TAC>& _instr, std::ofstream& os) : muncher(muncher), args_on_stack(0), instr(_instr), os(os) {
    bounds.clear();
    func_of_temp.clear();
    asm_name.clear();
}

void Assembler::assemble() {
    // global variables
    for (auto &tac : muncher.get_globals()) {
        auto name = tac.get_result().substr(1);
        os << "\t.globl " << name << "\n";
        os << "\t.data\n";
        os << name << ":" << std::setw(8) << ".quad " << std::stoi(tac.get_arg()) << "\n";
    }

    // compute the function where each temporary is defined
    int func_cnt = 0;
    for (auto &[start, finish] : muncher.procs_indexes()) {
        auto func_name = instr[start].get_result();

        for (auto i = start + 1; i <= finish; i++) {
            auto tac = instr[i];

            if (!tac.has_result() || tac.get_result()[0] != '%' || tac.get_result()[1] == '.') 
                continue;

            func_of_temp[tac.get_result()] = func_name;
        }
        
        for (auto &c : func_name)
            c = (c == ':' ? '_' : c);
        func_name += std::to_string(++func_cnt);
        asm_name[instr[start].get_result()] = instr[start].get_result() != "main" ? func_name : "main";
    }
    
    // compute how much we allocate on each function
    for (auto &[start, finish] : muncher.procs_indexes()) {
        process_proc(start, finish);
    }

    // procedures
    for (auto &[start, finish] : muncher.procs_indexes()) {
        auto name = asm_name[instr[start].get_result()];
        os << "\n";
        os << "\t.globl " << name << "\n";
        os << "\t.text\n";
        os << name << ":\n";
        assemble_proc(start, finish);
    }

    os.close();
}

void Assembler::process_proc(std::size_t start, std::size_t finish) {
    std::size_t mn = 1e9, mx = 0;
    auto func_name = instr[start].get_result();
    for (auto i = start + 1; i <= finish; i++) {
        auto t = instr[i];
        auto args = t.get_args();

#ifdef DEBUG
        std::cout << t << "\n";
#endif
        
        for (auto &temp : args) {
            if (func_of_temp[temp] == func_name) {
                auto num = static_cast<std::size_t>(std::stoi(temp.substr(1)));
                mn = std::min(mn, num);
                mx = std::max(mx, num);
            }
        }

        if (t.has_result()) {
            auto result = t.get_result();
            if (func_of_temp[result] == func_name) {
                auto num = static_cast<std::size_t>(std::stoi(result.substr(1)));
                mn = std::min(mn, num);
                mx = std::max(mx, num);
            }
        }
    }

    if (mx == 0) mn = mx = 0;

    bounds[func_name] = {mn, mx};
}

void Assembler::assemble_proc(std::size_t start, std::size_t finish) {
    // compute needed registers
    stack_size = 0;

    curr_func_name = instr[start].get_result();

    stack_size = bounds[curr_func_name].second - bounds[curr_func_name].first + 2 + instr[start].get_args().size();
    stack_size = (stack_size + 1) / 2 * 2;
    stack_offset = bounds[curr_func_name].first;

#ifdef DEBUG
    std::cout << stack_offset << "->" << bounds[curr_func_name].second << " ";
#endif

    os << "\tpushq %rbp\n";
    os << "\tmovq %rsp, %rbp\n";
    os << "\tsubq $" << 8 * stack_size << ", %rsp\n";

#ifdef DEBUG
    std::cout << stack_size << "\n";
#endif

    for (auto i = start + 1; i <= finish; i++) {
        assemble_instr(instr[i]);
    }
}

void Assembler::assemble_instr(TAC& tac) {
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
        auto result_temp = stack_register(tac.get_result());

        // convert function names to asm given names
        args[0] = std::isalpha(args[0][0]) ? asm_name[args[0]] : args[0];

        os << "\tmovq $" << args[0] << ", " << result_temp << "\n";
    }
    else if (op == "copy") {
        assert(args.size() >= 1 && tac.has_result());
        auto arg0_temp = stack_register(args[0]);
        auto result_temp = stack_register(tac.get_result());

        // special case, if we copy the static link, allocate it at -8(%rbp)
        if (args.size() == 2) {
            result_temp = "-8(%rbp)";
        }

        os << "\tmovq " << arg0_temp << ", %r10\n";
        os << "\tmovq %r10, " << result_temp << "\n";
    }
    else if (op == "call") {
        auto arg0_temp = stack_register(args[0]);
        os << "\tcall *" << arg0_temp << "\n";

        if (args_on_stack)
            os << "\taddq $" << 8 * ((args_on_stack + 1) / 2 * 2) << ", %rsp\n";
        args_on_stack = 0;

        if (tac.has_result()) {
            auto result_temp = stack_register(tac.get_result());
            os << "\tmovq %rax, " << result_temp << "\n";
        }
    }
    else if (op == "jmp") {
        assert(args.size() == 1 || (args.empty() && tac.has_result()));
        auto label = args.empty() ? tac.get_result() : args[0];
        os << "\tjmp " << label.substr(1) << "\n";
    }
    else if (jumps.count(op)) {
        assert(args.size() == 1 && tac.has_result());
        auto arg0_temp = stack_register(args[0]);
        os << "\tcmpq $0, " << arg0_temp << "\n";
        os << "\t" << op << " " << tac.get_result().substr(1) << "\n";
    }
    else if (auto it = uniops.find(op); it != uniops.end()) {
        assert(args.size() == 1 && tac.has_result());
        auto arg0_temp = stack_register(args[0]);
        auto result_temp = stack_register(tac.get_result());
        os << "\tmovq " << arg0_temp << ", %r10\n";
        os << "\t" << it->second << " %r10\n";
        os << "\tmovq %r10, " << result_temp << "\n";
    }
    else if (auto it = normal_binops.find(op); it != normal_binops.end()) {
        assert(args.size() == 2 && tac.has_result());
        auto arg0_temp = stack_register(args[0]);
        auto arg1_temp = stack_register(args[1]);
        auto result_temp = stack_register(tac.get_result());
        os << "\tmovq " << arg0_temp << ", %r10\n";
        os << "\t" << it->second << " " << arg1_temp << ", %r10\n";
        os << "\tmovq %r10, " << result_temp << "\n";
    }
    else if (auto it = special_binops.find(op); it != special_binops.end()) {
        assert(args.size() == 2 && tac.has_result());
        auto arg0_temp = stack_register(args[0]);
        auto arg1_temp = stack_register(args[1]);
        auto result_temp = stack_register(tac.get_result());
        it->second(arg0_temp, arg1_temp, result_temp, os);
    }
    else if (op == "ret") {
        if (!args.empty()) {
            auto arg0_temp = stack_register(args[0]);
            os << "\tmovq " << arg0_temp << ", %rax\n";
        } 
        else
            os << "\tmovq $0, %rax\n";
        os << "\tmovq %rbp, %rsp\n";
        os << "\tpopq %rbp\n";
        os << "\tretq\n";
    }
    else if (op == "param") {
        auto arg0_temp = stack_register(args[0]);
        auto id = std::stoi(tac.get_result());

        if (id <= 6)
            os << "\tmovq " << arg0_temp << ", " << arg_registers[id - 1] << "\n";
        else {
            os << "\tpushq " << arg0_temp << "\n";
            args_on_stack++;
        }
    }
    else if (op == "get_fp") {
        auto result_temp = stack_register(tac.get_result());
        os << "\tmovq %rbp, %r10\n";
        os << "\tmovq %r10, " << result_temp << "\n";
    }
    else {
        throw std::runtime_error("Unrecognized operator " + op);
    }
}

}; // namespace assembly