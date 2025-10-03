#include "cfg.h"
#include "../asm/asm.h"

namespace Opt {

Block::Block(std::vector<std::shared_ptr<TAC>>& instr) : instr(instr) {
    assert(instr[0]->get_opcode() == "label");
    label = instr[0]->get_arg();
    jumps.clear();

    // connections of current block
    for (auto &t : instr) {
        auto op = t->get_opcode();
        if (ASM::jumps.find(op) != ASM::jumps.end() || op == "jmp")
            jumps.push_back(t);
    }
}

[[nodiscard]] std::vector<Block> CFG::make_blocks(std::vector<TAC>& instr) {
    std::size_t i = 0;
    std::vector<Block> blocks;

    while (i < instr.size()) {
        assert(instr[i].get_opcode() == "label");
        std::vector<std::shared_ptr<TAC>> block_instr;

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
            block_instr.push_back(std::make_shared<TAC>(instr[i++]));
        
        // missing jmp at the end of the block
        auto last_instr = block_instr.back()->get_opcode();
        if (last_instr != "jmp" && last_instr != "ret") {
            block_instr.push_back(std::make_shared<TAC>(
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
    graph.clear();

    for (auto &block : blocks) {
        auto connections = block.get_jumps();
        auto label = block.get_label();

        for (auto &t : connections) {
            auto t_label = t->has_result() ? t->get_result() : t->get_arg();
            std::cout << label << "->" << t_label << "\n";
            graph[label].insert({t_label, t});
        }
    }
}

[[nodiscard]] std::vector<TAC> CFG::make_tac() {
    std::vector<TAC> instr;
    for (auto &block : blocks) {
        auto block_instr = block.get_instr();
        for (auto &t : block_instr)
            instr.push_back(*t);
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
    auto root = blocks[0].get_label();

#ifdef DEBUG
    std::cout << "Removing unreachable blocks from " << root << "\n";
#endif

    dfs(root, vis);

    std::vector<Block> temp_blocks;
    for (auto &block : blocks) {
        auto label = block.get_label();
        if (vis.find(label) != vis.end())
            temp_blocks.push_back(std::move(block));
#ifdef DEBUG
        else {
            std::cout << "Removed useless block " << label << "\n";
        }
#endif
    }

    blocks = std::move(temp_blocks);
}

void CFG::jt_seq_uncond() {
    bool found_chain = true;
    while (found_chain) {
        found_chain = false;
        for (auto &block : blocks) {
            auto label = block.get_label();
            // std::cout << label << "\n";
            for (auto [child_node, child] : graph[label]) {
                if (graph[child_node].size() == 1) {
                    // std::cout << label << " " << child_node << "\n";
                    auto child_block = get_block(child_node);
                    if (child_block.get_instr().size() == 2) {
                        // for (auto &t : child_block.get_instr())
                        //     std::cout << t << "\n";
                        // make jmp point to the label this dummy block points to
                        auto target_label = child_block.get_instr().back()->get_result(); 
                        child->set_result(target_label);
                        graph[label].erase(child_node);
                        graph[label][target_label] = child;
                        graph[child_node].clear();
                        // std::cout << "Found useless connexion between " << label << ", " << child_node << ", " << target_label << "\n";
                        // std::cout << child << " now\n";
                        // return;
                        found_chain = true;
                        break;
                    }
                }
            }
            if (found_chain) break;
        }

        // cleanup (maybe not needed every time?)
        uce();
    }
}

};