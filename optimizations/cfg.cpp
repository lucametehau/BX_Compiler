#include "cfg.h"
#include "../asm/asm.h"

namespace Opt {

Block::Block(std::vector<TAC>& instr) : instr(instr) {
    assert(instr[0].get_opcode() == "label");
    label = instr[0].get_args()[0];
    jumps.clear();

    // connections of current block
    for (auto &t : instr) {
        auto op = t.get_opcode();
        if (ASM::jumps.find(op) != ASM::jumps.end() || op == "jmp")
            jumps.push_back(t);
    }
}

[[nodiscard]] std::vector<Block> CFG::make_blocks(std::vector<TAC>& instr) {
    std::size_t i = 0;
    std::vector<Block> blocks;

    while (i < instr.size()) {
        assert(instr[i].get_opcode() == "label");
        std::vector<TAC> block_instr;

#ifdef DEBUG
        std::cerr << i << " start for block\n";
#endif

        auto j = i + 1;
        while (j < instr.size() && instr[j].get_opcode() != "label")
            j++;

#ifdef DEBUG
        std::cerr << j << " end for block\n";
#endif

        // probably a block between [i, j)
        while (i < j)
            block_instr.push_back(instr[i++]);
        
        // missing jmp at the end of the block
        auto last_instr = block_instr.back().get_opcode();
        if (last_instr != "jmp" && last_instr != "ret") {
            block_instr.push_back(TAC(
                "jmp",
                instr[j].get_args()
            ));
        }

        blocks.push_back(Block(block_instr));
    }

    return blocks;
}

void CFG::make_cfg(std::vector<TAC>& instr) {
    blocks = make_blocks(instr);
    for (auto &block : blocks) {
        auto connections = block.get_jumps();
        auto label = block.get_label();

        for (auto &t : connections) {
            auto t_label = t.has_result() ? t.get_result() : t.get_args()[0];
            graph[label].push_back({t_label, t});
        }
    }
}

[[nodiscard]] std::vector<TAC> CFG::make_tac() {
    std::vector<TAC> instr;
    for (auto &block : blocks) {
        auto block_instr = block.get_instr();
        for (auto &t : block_instr)
            instr.push_back(t);
    }
    return instr;
}

void CFG::dfs(std::string label, std::set<std::string>& vis) {
    vis.insert(label);

    for (auto &[child_label, edge] : graph[label]) {
        if (vis.find(child_label) != vis.end()) continue;

        dfs(child_label, vis);
    }
}

void CFG::uce() {
    std::set<std::string> vis;
    for (auto &block : blocks) {
        auto label = block.get_label();
        if (vis.find(label) == vis.end())
            dfs(label, vis);
    }

    std::vector<Block> temp_blocks;
    for (auto &block : blocks) {
        auto label = block.get_label();
        if (vis.find(label) != vis.end())
            temp_blocks.push_back(std::move(block));
    }

    blocks = std::move(temp_blocks);
}

void CFG::jt_seq_uncond() {
    while (true) {
        std::vector<Block> temp_blocks;

        for (auto &block : blocks) {
            auto label = block.get_label();
            for (auto &[child_node, child] : graph[label]) {
                if (graph[child_node].size() == 1) {
                    auto child_block = get_block(child_node);
                    if (child_block.get_instr().size() == 1) {
                        
                    }
                }
            }
        }
    }
}

};