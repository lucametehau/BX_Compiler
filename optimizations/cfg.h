#pragma once
#include "../mm/tac.h"
#include "../mm/mm.h"
#include "../asm/asm.h"
#include "block.h"
#include <set>
#include <cassert>
#include <memory>

namespace opt {

enum class OptimizationType {
    DEAD_COPY_REMOVAL,
    JT_SEQ_UNCOND,
    JT_COND_TO_UNCOND,
    COALESCE
};

class CFG {
private:
    std::vector<Block> blocks;
    std::map<Label, std::map<Label, std::shared_ptr<TAC>>> graph;
    std::map<std::string, std::string> original_temp;
    MM::MM& muncher;

    [[nodiscard]] std::vector<Block> make_blocks(std::vector<TAC>& instr);

    void dfs(Label node, std::set<Label>& vis);

    void build_liveness();

public:
    CFG(MM::MM& muncher) : muncher(muncher) {};

    void make_cfg(std::vector<TAC>& instr);

    [[nodiscard]] std::vector<TAC> make_tac();

    [[nodiscard]] Block& get_block(Label label) {
        for (auto &block : blocks) {
            if (block.get_label() == label)
                return block;
        }
        assert(false);
    }

    [[nodiscard]] std::vector<Label> get_predecessors(std::string label) {
        std::vector<Label> pred;
        for (auto &block : blocks) {
            auto pred_label = block.get_label();
            for (auto &[son, _] : graph[pred_label]) {
                if (son == label) {
                    pred.push_back(pred_label);
                    break;
                }
            }
        }

        return pred;
    }

    // unreachable code elimination
    void uce();

    // Block coalescing (block jumps unconditionally only to another block)
    void coalesce();

    // Jump Threading: Sequencing Unconditional Jumps
    void jt_seq_uncond();

    // Jump Threading: Turning Conditional into Unconditional Jumps
    void jt_cond_to_uncond();

    // Propagates temporaries from copies
    void copy_propagation();

    // Deletes dead copies (the result isn't used anywhere)
    void eliminate_dead_copies();

    // Generate crude SSA code
    void ssa_crude();
};


template <OptimizationType opt>
inline void optimize(MM::MM& muncher, std::vector<TAC> &instr, std::string file_prefix) {
    CFG cfg(muncher);
    cfg.make_cfg(instr);

    // apply optimization
    std::string suffix = "";
    if constexpr (opt == OptimizationType::DEAD_COPY_REMOVAL) {
        cfg.copy_propagation();
        cfg.eliminate_dead_copies();
        suffix = "deadcopy";
    }
    else if constexpr (opt == OptimizationType::JT_SEQ_UNCOND) {
        cfg.jt_seq_uncond();
        suffix = "jtseq";
    }
    else if constexpr (opt == OptimizationType::JT_COND_TO_UNCOND) {
        cfg.jt_cond_to_uncond();
        suffix = "jtcond";
    }
    else if constexpr (opt == OptimizationType::COALESCE) {
        cfg.coalesce();
        suffix = "coalesce";
    }

    // convert CFG to TAC instructions
    instr = cfg.make_tac();

    muncher.process(instr);
    // muncher.jsonify(file_prefix + "." + suffix + ".tac.json", instr);
    muncher.jsonify(file_prefix + ".tac.json", instr);

    // std::ofstream asm_file(file_prefix + "_" + suffix + ".s");
    std::ofstream asm_file(file_prefix + ".s");
    assembly::Assembler assembler(muncher, instr, asm_file);
    assembler.assemble();
}

};