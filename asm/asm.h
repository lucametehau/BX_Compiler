#pragma once
#include "tac.h"
#include <fstream>

namespace ASM {

class Assembler {
private:
    std::size_t stack_size;
    std::vector<TAC> instr;

public:
    Assembler(std::vector<TAC>& _instr);

    void assemble(std::ofstream& os);

private:
    std::string stack_register(std::string& temp) {
        assert(temp[0] == '%');
        return "-" + std::stoi(temp.substr(1)) + "(%rbp)";
    }
    
    void assemble_instr(std::ofstream& os, TAC& tac);
};

};