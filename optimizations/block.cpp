#include "block.h"
#include "../asm/asm.h"

namespace opt {

Block::Block(std::vector<std::shared_ptr<TAC>>& instr, bool start) : instr(instr), start(start) {
    assert(instr[start]->get_opcode() == "label");
    label = instr[start]->get_arg();
    jumps.clear();

    // connections of current block
    for (auto &t : instr) {
        auto op = t->get_opcode();
        if (assembly::jumps.find(op) != assembly::jumps.end() || op == "jmp")
            jumps.push_back(t);
    }
}

void Block::build_liveness([[maybe_unused]] Set &live_in_block, Set &live_out_block) {
    auto instr_count = instr.size();
    live_in.clear(), live_out.clear();
    live_in.resize(instr_count);
    live_out.resize(instr_count);

    auto temp_live = live_out_block;
    for (int i = instr_count - 1; i >= 0; i--) {
        auto tac = instr[i];
        live_out[i] = temp_live;
        live_in[i] = use[i].join(live_out[i].minus(def[i]));
        temp_live = live_in[i];
    }
}


void Block::build_def_use(Set &def_block, Set &use_block) {
    auto instr_count = instr.size();
    def.clear(), use.clear();
    def.resize(instr_count);
    use.resize(instr_count);

#ifdef DEBUG
    std::cout << "Building def_use for " << label << "\n";
#endif

    for (std::size_t i = 0; i < instr_count; i++) {
        auto tac = instr[i];
        auto opcode = tac->get_opcode();

        // label or jump instructions
        if (opcode == "label" || (tac->has_result() && tac->get_result()[1] == '.'))
            continue;

        for (auto &arg : tac->get_args()) {
            if (arg[0] == '%' && arg[1] != 'p')
                use[i].insert(arg);
        }

        if (tac->has_result() && tac->get_result()[0] == '%')
            def[i].insert(tac->get_result());

        def_block = def_block.join(def[i]);
        use_block = use_block.join(use[i]);
    }

#ifdef DEBUG
    std::cout << "def : ";
    for (auto &x : def_block.get_set())
        std::cout << x << " ";
    std::cout << "\nuse : ";
    for (auto &x : use_block.get_set())
        std::cout << x << " ";
    std::cout << "\n";
#endif
}

void Block::eliminate_dead_copies() {
    auto instr_count = instr.size();
    std::vector<std::shared_ptr<TAC>> new_instr;

    for (std::size_t i = 0; i < instr_count; i++) {
        auto tac = instr[i];
        if (tac->get_opcode() != "copy") {
            new_instr.push_back(tac);
            continue;
        }

        if (live_out[i].count(tac->get_result()))
            new_instr.push_back(tac);
    }

    instr = new_instr;
    // don't forget to recompute everything since instructions changed!!
}

};