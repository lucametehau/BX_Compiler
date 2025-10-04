#pragma once
#include "../mm/tac.h"
#include <set>
#include <cassert>
#include <memory>

namespace Opt {

class Block {
private:
    std::vector<std::shared_ptr<TAC>> instr;
    std::string label;
    std::vector<std::shared_ptr<TAC>> jumps; 

public:
    Block() = default;
    Block(std::vector<std::shared_ptr<TAC>>& instr);

    [[nodiscard]] std::vector<std::shared_ptr<TAC>> get_jumps() {
        return jumps;
    }

    [[nodiscard]] std::string get_label() {
        return label;
    }

    [[nodiscard]] std::vector<std::shared_ptr<TAC>>& get_instr() {
        return instr;
    }
};

class CFG {
private:
    std::vector<Block> blocks;
    std::map<std::string, std::map<std::string, std::shared_ptr<TAC>>> graph;
    std::map<std::string, std::string> original_temp;

    [[nodiscard]] std::vector<Block> make_blocks(std::vector<TAC>& instr);

    void dfs(std::string node, std::set<std::string>& vis);

public:
    CFG() = default;

    void make_cfg(std::vector<TAC>& instr);

    [[nodiscard]] std::vector<TAC> make_tac();

    [[nodiscard]] Block& get_block(std::string label) {
        for (auto &block : blocks) {
            if (block.get_label() == label)
                return block;
        }
        assert(false);
    }

    // unreachable code elimination
    void uce();

    // Jump Threading: Sequencing Unconditional Jumps
    void jt_seq_uncond();

    // Jump Threading: Turning Conditional into Unconditional Jumps
    void jt_cond_to_uncond();
};

};