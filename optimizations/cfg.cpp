#include "cfg.h"
#include "../asm/asm.h"

namespace Opt {

Block::Block(std::vector<std::shared_ptr<TAC>>& instr, bool start) : instr(instr), start(start) {
    assert(instr[start]->get_opcode() == "label");
    label = instr[start]->get_arg();
    jumps.clear();

    // connections of current block
    for (auto &t : instr) {
        auto op = t->get_opcode();
        if (ASM::jumps.find(op) != ASM::jumps.end() || op == "jmp")
            jumps.push_back(t);
    }
}

[[nodiscard]] std::vector<Block> CFG::make_blocks(std::vector<TAC>& instr) {
    std::vector<Block> blocks;

    for (auto &[start, finish] : muncher.procs_indexes()) {
        std::size_t i = start + 1;
        while (i <= finish) {
            assert(instr[i].get_opcode() == "label");
            std::vector<std::shared_ptr<TAC>> block_instr;

            if (i == start + 1)
                block_instr.push_back(std::make_shared<TAC>(instr[start]));

    #ifdef DEBUG
            std::cerr << instr[i] << ", " << i << " start for block\n";
    #endif

            auto j = i + 1;
            while (j <= finish && instr[j].get_opcode() != "label")
                j++;

    #ifdef DEBUG
            std::cerr << instr[j - 1] << ", " << j - 1 << " end for block\n";
    #endif

            // probably a block between [i, j)
            while (i < j)
                block_instr.push_back(std::make_shared<TAC>(instr[i++]));
            
            // missing jmp at the end of the block
            auto last_instr = block_instr.back()->get_opcode();
            if (last_instr != "jmp" && last_instr != "ret") {
                block_instr.push_back(std::make_shared<TAC>(
                    "jmp",
                    std::vector<std::string>{},
                    instr[j].get_arg()
                ));
            }

            blocks.push_back(Block(block_instr, block_instr[0]->get_opcode() == "proc"));
        }

        // build the inheritance tree of temporaries
        for (i = start + 1; i <= finish; i++) {
            auto t = instr[i];
            auto op = t.get_opcode();

            // normal operations between temporaries only
            if (op != "label" && op != "jmp" && ASM::jumps.find(op) == ASM::jumps.end() && t.has_result()) {
                if (op == "copy") {
                    auto arg = t.get_arg();
                    if (original_temp.find(arg) == original_temp.end())
                        original_temp[arg] = arg;
                    original_temp[t.get_result()] = original_temp[arg];
                }
                else if (original_temp.find(t.get_result()) == original_temp.end())
                    original_temp[t.get_result()] = t.get_result();
            }
        }
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
#ifdef DEBUG
            std::cout << label << "->" << t_label << "\n";
#endif
            graph[label].insert({t_label, t});
        }
    }
}

[[nodiscard]] std::vector<TAC> CFG::make_tac() {
    std::vector<TAC> instr;

    std::cout << "Making TAC from CFG\n";

    // treat globals separately
    for (auto &global : muncher.get_globals())
        instr.push_back(global);

    for (auto &block : blocks) {
        auto block_instr = block.get_instr();
        for (auto &t : block_instr)
            instr.push_back(*t), std::cout << *t << "\n";
    }
    std::cout << "--------------------------------------------------\n";
    // exit(0);
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

    std::size_t ind = 0;
    for (std::size_t i = 0; i < muncher.procs_indexes().size(); i++) {
        // find block matching the starting label of the procedure
        while (ind < blocks.size() && !blocks[ind].is_starting())
            ind++;

        auto root = blocks[ind].get_label();

    #ifdef DEBUG
        std::cout << "Removing unreachable blocks from " << root << "\n";
    #endif

        dfs(root, vis);

        ind++;
    }

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

            for (auto [child_node, tac] : graph[label]) {
                if (graph[child_node].size() == 1) {
                    auto child_block = get_block(child_node);

                    if (child_block.get_instr().size() == 2) {
                        // make jmp point to the label this dummy block points to
                        auto target_label = child_block.get_instr().back()->get_result(); 
                        tac->set_result(target_label);
                        graph[label].erase(child_node);
                        graph[label][target_label] = tac;

#ifdef DEBUG
                        std::cout << label << "->" << child_node << "->" << target_label << "\n";
#endif
                        
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

void CFG::jt_cond_to_uncond() {
    bool found_cond = true;
    while (found_cond) {
        found_cond = false;

        for (auto &block : blocks) {
            auto label = block.get_label();
            for (auto [child_node, tac] : graph[label]) {
                // only conditional jumps
                auto jump = tac->get_opcode();
                if (ASM::jumps.find(jump) == ASM::jumps.end())
                    continue;
                
                auto &child_block = get_block(child_node);
                auto &child_instr = child_block.get_instr();

                for (std::size_t i = 0; i < child_instr.size(); i++) {
                    // we entered the child block with like
                    // jc %1, %.La
                    // then we have
                    // jc %1, %.Lb
                    // so simply replace it with jmp and delete code after
                    auto &temp_tac = child_instr[i];
#ifdef DEBUG
                    if (temp_tac->get_opcode() == jump) {
                        std::cout << *temp_tac << "\n";
                        std::cout << *tac << "\n";
                        std::cout << original_temp[temp_tac->get_arg()] << "\n";
                        std::cout << original_temp[tac->get_arg()] << "\n";
                    }
#endif
                    if (temp_tac->get_opcode() == jump && original_temp[temp_tac->get_arg()] == original_temp[tac->get_arg()]) {
                        found_cond = true;

                        child_instr[i] = std::make_shared<TAC>(
                            "jmp",
                            std::vector<std::string>{},
                            temp_tac->get_result()
                        );
                        child_instr.resize(i + 1);
                        break;
                    }
                }
            }
            if (found_cond) break;
        }

        // cleanup (maybe not needed every time?)
        uce();
    }
}

};