#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer/lexer.h"
#include "ast/declarations.h"
#include "asm/asm.h"
#include "optimizations/cfg.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Wrong usage! Call " << argv[0] << " [filename.bx]\n";
        return 1;
    }

    const std::string filename(argv[1]);
    const std::string file_prefix = filename.substr(0, filename.find("."));
    std::cout << "Hello! Lexing file " << filename << "...\n";
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
    

    Lexer::Lexer lexer(get_file_content(in));
    auto tokens = lexer.tokenize();

    Parser::Parser parser(tokens);
    std::cout << "Parsing and building AST...\n";
    auto ast = Grammar::Declarations::Program::match(parser);

    if (!ast) {
        throw std::runtime_error("Parser failed!");
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

    // auto cfg = Opt::CFG();
    // cfg.make_cfg(instr);

    // for (int i = 1; i <= 10; i++) {
    //     cfg.jt_seq_uncond();

    //     std::vector<TAC> new_instr = cfg.make_tac();

    //     std::ofstream asm_file(file_prefix + ".s");
    //     ASM::Assembler assembler(new_instr);
    //     assembler.assemble(asm_file);

    //     muncher.jsonify(file_prefix + ".opt1.tac.json", new_instr);

    //     cfg.jt_cond_to_uncond();

    //     std::vector<TAC> new_new_instr = cfg.make_tac();

    //     muncher.jsonify(file_prefix + ".opt2.tac.json", new_new_instr);
    // }
    return 0;
}