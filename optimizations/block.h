#pragma once
#include <set>
#include <memory>
#include "../mm/tac.h"
#include "../mm/mm.h"
#include "../utils/utils.h"

namespace opt {

using Set = utils::GeneralSet<std::string>;
using Label = std::string;

class Block {
private:
    std::vector<std::shared_ptr<TAC>> instr;
    Label label;
    std::vector<std::shared_ptr<TAC>> jumps;
    std::vector<Set> live_in, live_out; // liveness for each temporary
    std::vector<Set> def, use;
    bool start;

public:
    Block() = default;
    Block(std::vector<std::shared_ptr<TAC>>& instr, bool start);

    void set_instr(std::vector<std::shared_ptr<TAC>>& _instr) {
        instr = _instr;
    } 

    [[nodiscard]] std::vector<std::shared_ptr<TAC>>& get_instr() {
        return instr;
    }

    [[nodiscard]] Label get_label() {
        return label;
    }

    [[nodiscard]] std::vector<std::shared_ptr<TAC>> get_jumps() {
        return jumps;
    }

    [[nodiscard]] Set get_live_out(size_t ind) {
        return live_out[ind];
    }

    [[nodiscard]] bool is_starting() const {
        return start;
    }

    void build_liveness(Set &live_in_block, Set &live_out_block);

    void build_def_use(Set &def_block, Set &use_block);

    void remove_instr(std::size_t ind);
};

};