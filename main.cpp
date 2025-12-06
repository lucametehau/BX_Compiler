#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include "lexer/lexer.h"
#include "ast/declarations.h"
#include "asm/asm.h"
#include "optimizations/cfg.h"

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::cout << "Wrong usage! Call " << argv[0] << " [filename.bx] <-fenable-opt>\n";
        return 1;
    }

    const std::string filename(argv[1]);
    std::cout << "Hello! Lexing file " << filename << "...\n";

#ifdef TEST
    int pos = filename.size() - 1;

    while (pos >= 0 && filename[pos] != '/') 
        pos--;

    const std::string file_prefix = filename.substr(pos+1, filename.find(".", pos) - pos-1);
#else   
    const std::string file_prefix = filename.substr(0, filename.find("."));
#endif

    std::ifstream in(filename);

    if (!in) {
        std::cout << "Unknown file " << filename << "!\n";
        return 1;
    }

    auto get_file_content = [&](std::ifstream& in) {
        std::ostringstream oss;
        oss << in.rdbuf();
        return oss.str();
    };
    

    lexer::Lexer lexer(get_file_content(in));
    auto tokens = lexer.tokenize();

    parser::Parser parser(tokens);
    std::cout << "Parsing and building AST...\n";
    auto ast = Grammar::Declarations::Program::match(parser);

    if (!ast) {
        throw std::runtime_error("Parser failed!");
    }

    if (!parser.finished()) {
        throw std::runtime_error(std::format(
            "Parser failed at row {}, col {}!", parser.peek().get_row(), parser.peek().get_col()
        ));
    }

#ifdef DEBUG
    ast->print(std::cout);
#endif

    MM::MM muncher;

    // type check
    std::cout << "Type checking AST...\n";
    ast->type_check(muncher);

    // munch instructions
    std::cout << "Munching AST...\n";
    auto instr = ast->munch(muncher);
    muncher.jsonify(file_prefix + ".tac.json", instr); 

    // for (auto &tac : instr)
    //     std::cout << tac << "\n";
    
    // assembling
    std::cout << "Assembling...\n";
    std::ofstream asm_file(file_prefix + ".s");
    assembly::Assembler assembler(muncher, instr, asm_file);
    assembler.assemble();

    if (argc == 3 && !std::strcmp(argv[2], "-fenable-opt")) {
        std::cout << "Applying optimizations...\n";

        for (int i = 1; i <= 1; i++) {
            std::cout << std::format("Before round #{} of optimizations, we have {} operations.\n", i, instr.size());

            opt::optimize<opt::OptimizationType::DEAD_COPY_REMOVAL>(muncher, instr, file_prefix);
            opt::optimize<opt::OptimizationType::JT_SEQ_UNCOND>(muncher, instr, file_prefix);
            opt::optimize<opt::OptimizationType::JT_COND_TO_UNCOND>(muncher, instr, file_prefix);
            opt::optimize<opt::OptimizationType::COALESCE>(muncher, instr, file_prefix);
            std::cout << std::format("After round #{} of optimizations, we have {} operations.\n", i, instr.size());
        }
    }
    return 0;
}