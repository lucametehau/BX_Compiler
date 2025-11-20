#include "cfg.h"
#include "../asm/asm.h"
#include <algorithm>

namespace opt {

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
            if (op != "label" && op != "jmp" && assembly::jumps.find(op) == assembly::jumps.end() && t.has_result()) {
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

#ifdef DEBUG
    std::cout << "Making TAC from CFG\n";
#endif

    // treat globals separately
    for (auto &global : muncher.get_globals())
        instr.push_back(global);

    for (auto &block : blocks) {
        auto block_instr = block.get_instr();
        for (auto &t : block_instr) {
            instr.push_back(*t);
#ifdef DEBUG
            std::cout << *t << "\n";
#endif
        }
    }

#ifdef DEBUG
    std::cout << "--------------------------------------------------\n";
#endif
    // exit(0);
    return instr;
}

void CFG::dfs(Label label, std::set<Label>& vis) {
    vis.insert(label);

    for (auto &[child_label, edge] : graph[label]) {
        if (vis.find(child_label) != vis.end()) continue;

        dfs(child_label, vis);
    }
}

void CFG::uce() {
    std::set<Label> vis;

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

void CFG::coalesce() {
    bool found = true;
    while (found) {
        found = false;

        // compute in degrees
        std::map<Label, int> in_deg;
        for (auto &block : blocks) {
            auto label = block.get_label();

            for (auto [child_node, tac] : graph[label]) {
                in_deg[child_node]++;
            }
        }

        for (auto &block : blocks) {
            auto label = block.get_label();

            if (graph[label].size() != 1)
                continue;

            auto [child_label, tac] = *graph[label].begin();

            if (in_deg[child_label] != 1)
                continue;

            // we have
            // L0: ....
            //    jmp L1
            // L1: ....
            // and we can only reach L1 from L0
            // hence we can coalesce the blocks

            // remove jmp at the end
            block.get_instr().pop_back();

            auto child_instr = get_block(child_label).get_instr();

            // remove label at thee beginning
            child_instr.erase(child_instr.begin());
            utils::concat(block.get_instr(), child_instr);

            // assign all sons of child_label to current label
            graph[label].erase(child_label);
            for (auto &[child_label, tac] : graph[child_label])
                graph[label][child_label] = tac;
            graph[child_label].clear();

#ifdef DEBUG
            std::cout << "Coalesced " << label << "->" << child_label << "\n";
#endif
            
            found = true;
            break;
        }

        // cleanup (maybe not needed every time?)
        uce();
    }
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
                if (assembly::jumps.find(jump) == assembly::jumps.end())
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

void CFG::build_liveness() {
    std::map<Label, Set> live_in_block, live_out_block;
    std::map<Label, Set> def_block, use_block;

    // build def and use sets
    for (auto &block : blocks) {
        block.build_def_use(def_block[block.get_label()], use_block[block.get_label()]);
    }

    // liveness of each block
    {
        bool changed = true;
        while (changed) {
            changed = false;
            for (auto it = blocks.rbegin(); it != blocks.rend(); it++) {
                auto &block = *it;
                auto label = block.get_label();
                auto temp_in = live_in_block[label];
                auto temp_out = live_out_block[label];

                live_out_block[label] = Set();
                for (auto &[succ_label, _] : graph[label]) {
                    live_out_block[label] = live_out_block[label].join(live_in_block[succ_label]);
                }

                live_in_block[label] = use_block[label].join(live_out_block[label].minus(def_block[label]));
                if (live_in_block[label] != temp_in || live_out_block[label] != temp_out)
                    changed = true;
            }
        }
    }

    // liveness of each instruction
    for (auto &block : blocks) {
        block.build_liveness(live_in_block[block.get_label()], live_out_block[block.get_label()]);
    }
}

void CFG::ssa_crude() {
    // build liveness
    std::map<Label, Set> live_in_block, live_out_block;
    std::map<Label, Set> def_block, use_block;

    // build def and use sets
    for (auto &block : blocks) {
        block.build_def_use(def_block[block.get_label()], use_block[block.get_label()]);
    }

    // liveness of each block
    {
        bool changed = true;
        while (changed) {
            changed = false;
            for (auto it = blocks.rbegin(); it != blocks.rend(); it++) {
                auto &block = *it;
                auto label = block.get_label();
                auto temp_in = live_in_block[label];
                auto temp_out = live_out_block[label];

                live_out_block[label] = Set();
                for (auto &[succ_label, _] : graph[label]) {
                    live_out_block[label] = live_out_block[label].join(live_in_block[succ_label]);
                }

                live_in_block[label] = use_block[label].join(live_out_block[label].minus(def_block[label]));
                if (live_in_block[label] != temp_in || live_out_block[label] != temp_out)
                    changed = true;
            }
        }
    }

    // liveness of each instruction
    for (auto &block : blocks) {
        block.build_liveness(live_in_block[block.get_label()], live_out_block[block.get_label()]);
    }
    
    // add phi functions at block entries based on live-in sets
    for (auto &block : blocks) {
        auto label = block.get_label();
        auto preds = get_predecessors(label);
        auto live_in = live_in_block[label];
        
        std::vector<std::shared_ptr<TAC>> new_instr;
        for (auto &temp : live_in.get_set()) {
            std::map<Label, std::string> phi_args;
            for (auto &pred_label : preds) {
                phi_args[pred_label] = temp;
            }
            new_instr.push_back(std::make_shared<TAC>(
                "phi",
                phi_args,
                ""
            ));
        }
        
        // insert phi instructions at the beginning of the block
        auto &instr = block.get_instr();
        instr.insert(instr.begin(), new_instr.begin(), new_instr.end());
    }
    
    // temporary versioning
    std::map<std::string, int> version_counter;
    std::map<Label, std::map<std::string, std::string>> version_maps;
    
    // version_maps["entry"] = {};
    
    for (auto &block : blocks) {
        auto label = block.get_label();
        auto &instr = block.get_instr();
        std::map<std::string, std::string> &ver_map = version_maps[label];
        
        for (auto &tac_ptr : instr) {
            auto &t = *tac_ptr;
            
            for (auto &arg : t.get_args()) {
                if (arg[0] == '%' && ver_map.count(arg)) {
                    arg = ver_map[arg];
                }
            }
            
            if (t.get_opcode() == "phi") {
                auto &phi_args = t.get_phi_args();
                for (auto &[pred_label, temp] : phi_args) {
                    if (version_maps.count(pred_label) && version_maps[pred_label].count(temp)) {
                        phi_args[pred_label] = version_maps[pred_label][temp];
                    }
                }
            }
            
            if (t.has_result() && t.get_result()[0] == '%') {
                std::string root = t.get_result();
                if (version_counter.count(root) == 0) {
                    version_counter[root] = 0;
                }
                std::string new_temp = root + "." + std::to_string(version_counter[root]++);
                t.set_result(new_temp);
                ver_map[root] = new_temp;
            }
        }
    }
    
    for (auto &block : blocks) {
        auto &instr = block.get_instr();
        for (auto &tac_ptr : instr) {
            if (tac_ptr->get_opcode() == "phi") {
                auto &phi_args = tac_ptr->get_phi_args();
                for (auto &[pred_label, temp] : phi_args) {
                    if (version_maps.count(pred_label)) {
                        std::string root = temp;
                        size_t dot_pos = root.find('.');
                        if (dot_pos != std::string::npos) {
                            root = root.substr(0, dot_pos);
                        }
                        if (version_maps[pred_label].count(root)) {
                            phi_args[pred_label] = version_maps[pred_label][root];
                        }
                    }
                }
            }
        }
    }
}

void CFG::copy_propagation() {
    bool changed = true;
    while (changed) {
        changed = false;
        
        // copy map: for each copy instruction %u = copy %v, map u -> v
        std::map<std::string, std::string> copy_map;
        for (auto &block : blocks) {
            auto &instr = block.get_instr();
            for (auto &tac_ptr : instr) {
                if (tac_ptr->get_opcode() == "copy" && tac_ptr->has_result() && tac_ptr->get_arg()[1] != 'p') {
                    std::string from = tac_ptr->get_arg();  // Assuming single arg for copy
                    std::string to = tac_ptr->get_result();
                    copy_map[to] = from;
                }
            }
        }
        
        for (auto &block : blocks) {
            auto &instr = block.get_instr();
            for (auto &tac_ptr : instr) {
                if (tac_ptr->get_opcode() == "phi") {
                    auto &phi_args = tac_ptr->get_phi_args();
                    for (auto &[label, temp] : phi_args) {
                        if (copy_map.count(temp)) {
                            phi_args[label] = copy_map[temp];
                            changed = true;
                        }
                    }
                } else {
                    for (auto &arg : tac_ptr->get_args()) {
                        if (copy_map.count(arg)) {
                            arg = copy_map[arg];
                            changed = true;
                        }
                    }
                }
                
                if (tac_ptr->has_result() && tac_ptr->get_opcode() != "copy" && copy_map.count(tac_ptr->get_result())) {
                    tac_ptr->set_result(copy_map[tac_ptr->get_result()]);
                    changed = true;
                }
            }
        }
        
        for (auto &block : blocks) {
            auto &instr = block.get_instr();
            instr.erase(std::remove_if(instr.begin(), instr.end(), 
                [](const std::shared_ptr<TAC>& tac) {
                    return tac->get_opcode() == "copy" && 
                           tac->has_result() && 
                           tac->get_result() == tac->get_arg();
                }), instr.end());
        }
    }
}

void CFG::eliminate_dead_copies() {
    bool changed = true;
    while (changed) {
        build_liveness();
        changed = false;
        for (auto &block : blocks) {
            auto &instr = block.get_instr();
            auto label = block.get_label();
            std::vector<std::shared_ptr<TAC>> new_instr;

            for (std::size_t i = 0; i < instr.size(); i++) {
                auto tac = instr[i];
                if (tac->get_opcode() != "copy") {
                    new_instr.push_back(tac);
                    continue;
                }

                bool is_dead = true;
                auto result = tac->get_result();
                if (block.get_live_out(i).count(result))
                    is_dead = false;
                else {
                    for (auto &[son, _] : graph[block.get_label()]) {
                        auto &succ_block = get_block(son);
                        for (auto &succ_tac : succ_block.get_instr()) {
                            if (succ_tac->get_opcode() == "phi") {
                                for (auto &[pred_label, temp] : succ_tac->get_phi_args()) {
                                    if (pred_label == block.get_label() && temp == result) {
                                        is_dead = false;
                                        break;
                                    }
                                }
                            }
                            if (!is_dead) break;
                        }
                        if (!is_dead) break;
                    }
                }

                if (!is_dead)
                    new_instr.push_back(tac);
                else
                    changed = true;
            }

            block.set_instr(new_instr);
        }
    }
}

};