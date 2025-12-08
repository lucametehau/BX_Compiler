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

    // auto temp_ind = 0;
    std::map<MM::Temporary, MM::Temporary> new_temp;

    auto resolve_temp = [&](MM::Temporary temp) {
        if (temp[0] == '%' && std::isdigit(temp[1])) {
            muncher.get_func_of_temps()[temp] = muncher.get_func_of_temps()[temp.substr(0, temp.find('.'))];
        }
        return temp;
    };

    auto resolve_instr = [&](TAC& tac) {
        for (auto &arg : tac.get_args()) {
            arg = resolve_temp(arg);
        }

        if (tac.has_result()) {
            tac.set_result(resolve_temp(tac.get_result()));
        }
    };

    // treat globals separately
    for (auto &global : muncher.get_globals()) {
        resolve_instr(global);
        instr.push_back(global);
    }

    for (auto &block : blocks) {
        auto block_instr = block.get_instr();
        for (auto &t : block_instr) {
            // std::cout << "Before " << *t << " ";
            resolve_instr(*t);
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
    // =========================================================
    // PART 1: LIVENESS ANALYSIS (Needed to find where to put Phis)
    // =========================================================
    std::map<Label, Set> live_in_block, live_out_block;
    std::map<Label, Set> def_block, use_block;

    for (auto &block : blocks) {
        block.build_def_use(def_block[block.get_label()], use_block[block.get_label()]);
    }

    // Iterative liveness
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto it = blocks.rbegin(); it != blocks.rend(); it++) {
            auto &block = *it;
            auto label = block.get_label();
            auto temp_in = live_in_block[label];

            live_out_block[label] = Set();
            for (auto &[succ_label, _] : graph[label]) {
                live_out_block[label] = live_out_block[label].join(live_in_block[succ_label]);
            }

            live_in_block[label] = use_block[label].join(live_out_block[label].minus(def_block[label]));
            
            if (live_in_block[label] != temp_in) changed = true;
        }
    }

    // =========================================================
    // PART 2: INSERT PHI NODES
    // =========================================================
    for (auto &block : blocks) {
        if (block.is_starting()) continue;

        auto label = block.get_label();
        auto preds = get_predecessors(label);
        auto live_in = live_in_block[label];
        
        std::vector<std::shared_ptr<TAC>> new_instr;
        
        // Skip label instruction
        auto insert_pos = block.get_instr().begin();
        if (insert_pos != block.get_instr().end()) insert_pos++;

        for (auto &temp : live_in.get_set()) {
            // Only process user temporaries (starts with %) and ignore params (%p)
            if (temp[0] == '%' && temp[1] != 'p' && temp[1] != '.') {
                std::map<Label, std::string> phi_args;
                // Initialize args with the original name as a placeholder
                for (auto &p : preds) phi_args[p] = temp;
                
                new_instr.push_back(std::make_shared<TAC>("phi", phi_args, temp));
            }
        }
        block.get_instr().insert(insert_pos, new_instr.begin(), new_instr.end());
    }

    // =========================================================
    // PART 3: RENAMING (DFS + Stack)
    // =========================================================
    std::map<std::string, int> version_counter;
    std::map<std::string, std::vector<std::string>> stacks;
    std::set<Label> visited;

    auto new_name = [&](std::string original) {
        if (version_counter.find(original) == version_counter.end())
            version_counter[original] = 0;
        return original + "." + std::to_string(version_counter[original]++);
    };

    std::function<void(Label)> rename = [&](Label label) {
        if (visited.count(label)) return;
        visited.insert(label);

        auto &block = get_block(label);
        std::map<std::string, int> pushed_count; // For popping later

        // 3a. Rename Phi Results (Definitions)
        for (auto &tac : block.get_instr()) {
            if (tac->get_opcode() != "phi") continue;
            
            std::string original = tac->get_result();
            std::string fresh = new_name(original);
            tac->set_result(fresh);
            
            stacks[original].push_back(fresh);
            pushed_count[original]++;
        }

        // 3b. Rename Body Instructions
        for (auto &tac : block.get_instr()) {
            if (tac->get_opcode() == "phi") continue;

            // Rename Uses (Args)
            for (auto &arg : tac->get_args()) {
                if (arg[0] == '%' && arg[1] != 'p' && arg[1] != '.') {
                    // Check if we have a version on stack for this variable
                    // We check if the 'root' (e.g., %8) has a stack
                    std::string root = arg; 
                    // Note: If arg is already versioned (rare in crude), strip suffix. 
                    // But here inputs are raw.
                    if (!stacks[root].empty()) {
                        arg = stacks[root].back();
                    }
                }
            }

            // Rename Definition (Result)
            if (tac->has_result()) {
                std::string res = tac->get_result();
                if (res[0] == '%' && res[1] != 'p' && res[1] != '.') {
                    std::string fresh = new_name(res);
                    tac->set_result(fresh);
                    stacks[res].push_back(fresh);
                    pushed_count[res]++;
                }
            }
        }

        // 3c. Update Successors' Phi Args (The Loop Fix)
        for (auto &[succ_label, _] : graph[label]) {
            auto &succ_block = get_block(succ_label);
            for (auto &tac : succ_block.get_instr()) {
                if (tac->get_opcode() != "phi") continue;

                auto &phi_args = tac->get_phi_args();
                // If this successor expects an input from 'label' (us)
                if (phi_args.count(label)) {
                    std::string root = phi_args[label]; // Placeholder holds root name
                    if (!stacks[root].empty()) {
                        phi_args[label] = stacks[root].back();
                    }
                }
            }
        }

        // 3d. Recurse
        for (auto &[succ_label, _] : graph[label]) {
            rename(succ_label);
        }

        // 3e. Pop Stacks (Backtracking)
        for (auto const& [var, count] : pushed_count) {
            for(int i=0; i<count; ++i) stacks[var].pop_back();
        }
    };

    // Start renaming from entry
    for (auto &block : blocks) {
        if (block.is_starting()) {
            rename(block.get_label());
            break;
        }
    }

    std::cout << "aaaa\n";
}

void CFG::copy_propagation() {
    bool changed = true;
    while (changed) {
        changed = false;
        std::map<std::string, std::string> replacements;

        // 1. Gather all copies across ALL blocks
        for (auto &block : blocks) {
            for (auto &tac : block.get_instr()) {
                // If we find %x = copy %y
                if (tac->get_opcode() == "copy" && tac->get_args().size() == 1) {
                    std::string to = tac->get_result();
                    std::string from = tac->get_arg();
                    
                    // Don't propagate if copying a parameter register (optional safeguard)
                    if (from[1] == 'p') continue;

                    // Record replacement: %x -> %y
                    replacements[to] = from;
                }
            }
        }

        if (replacements.empty()) break;

        // 2. Apply replacements to usages globally
        for (auto &block : blocks) {
            for (auto &tac : block.get_instr()) {
                // Replace in standard arguments
                for (auto &arg : tac->get_args()) {
                    // While loop handles chains like A->B->C
                    while (replacements.count(arg)) {
                        arg = replacements[arg];
                        changed = true;
                    }
                }

                // Replace in PHI arguments
                if (tac->get_opcode() == "phi") {
                    auto &phi_args = tac->get_phi_args();
                    for (auto &[label, arg] : phi_args) {
                        while (replacements.count(arg)) {
                            arg = replacements[arg];
                            changed = true;
                        }
                    }
                }
            }
        }
    }

    // propagate phis now

    

    for (auto &block : blocks) {
        auto &instr = block.get_instr();
        
        for (auto &tac : instr) {
            if (tac->get_opcode() == "phi") {
                std::string dest = tac->get_result();
                
                // For each predecessor (source of the value)
                for (auto &[pred_label, src_temp] : tac->get_phi_args()) {
                    // Find the predecessor block
                    auto &pred_block = get_block(pred_label);
                    auto &pred_instr = pred_block.get_instr();
                    
                    // We must insert the copy at the END of the predecessor, 
                    // but BEFORE the jump/branch instruction.
                    auto insert_pos = pred_instr.end();
                    
                    // Move iterator back to skip jumps/branches/labels at the end
                    // (Adjust this condition based on exactly what your jumps look like)
                    while (insert_pos != pred_instr.begin()) {
                        auto prev = std::prev(insert_pos);
                        std::string op = (*prev)->get_opcode();
                        if (op == "jmp" || op == "jg" || op == "jl" || op == "je" || op == "ret" || op == "call") {
                            insert_pos--;
                        } else {
                            break;
                        }
                    }

                    // Insert: %dest = copy %src_temp
                    pred_instr.insert(insert_pos, std::make_shared<TAC>(
                        "copy", std::vector<std::string>{src_temp}, dest
                    ));
                }
            }
        }
    }

    // 2. Now it is safe to delete all Phis
    for (auto &block : blocks) {
        auto &instr = block.get_instr();
        for (auto it = instr.begin(); it != instr.end(); ) {
            if ((*it)->get_opcode() == "phi") {
                it = instr.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void CFG::eliminate_dead_copies() {
    for (auto &block : blocks) {
        std::cout << block.get_label() << "\n";
        for (auto &instr : block.get_instr()) {
            std::cout << *instr << "\n";
        }
    }
    std::cout << "Before eliminating\n";
    bool changed = true;
    int op = 100;
    while (changed && --op > 0) {
        changed = false;
        std::map<std::string, int> use_count;

        std::cout << "HUH\n";
        // 1. Count uses of all temporaries
        for (auto &block : blocks) {
            for (auto &tac : block.get_instr()) {
                // Standard args
                for (auto &arg : tac->get_args()) {
                    if (arg[0] == '%') use_count[arg]++;
                }
                // Phi args
                if (tac->get_opcode() == "phi") {
                    for (auto &[label, arg] : tac->get_phi_args()) {
                        if (arg[0] == '%') use_count[arg]++;
                    }
                }
            }
        }

        std::cout << "WHERE\n";

        // 2. Remove dead instructions
        for (auto &block : blocks) {
            auto &instr = block.get_instr();
            // Use iterator to safely erase while looping
            for (auto it = instr.begin(); it != instr.end(); ) {
                auto tac = *it;
                std::cout << tac << "\n";
                
                // Check if it's a copy instruction
                if (tac->get_opcode() == "copy" && tac->has_result()) {
                    std::string res = tac->get_result();
                    
                    // If result is never used, delete the instruction
                    if (use_count[res] == 0) {
                        it = instr.erase(it);
                        changed = true;
                        continue; // Skip the increment
                    }
                }
                
                ++it;
            }
        }

        std::cout << "After iteration " << op << "\n";
        for (auto &block : blocks) {
            std::cout << block.get_label() << "\n";
            for (auto &instr : block.get_instr()) {
                std::cout << *instr << "\n";
            }
        }
    }

    std::cout << "After eliminating " << op << "\n";
    for (auto &block : blocks) {
        std::cout << block.get_label() << "\n";
        for (auto &instr : block.get_instr()) {
            std::cout << *instr << "\n";
        }
    }

    std::cout << "After eliminating EVERYTHING ------------------------------------------------- " << op << "\n";
    for (auto &block : blocks) {
        std::cout << block.get_label() << "\n";
        for (auto &instr : block.get_instr()) {
            std::cout << *instr << "\n";
        }
    }
}

};