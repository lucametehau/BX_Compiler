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
    
    // assembling
    std::cout << "Assembling...\n";
    std::ofstream asm_file(file_prefix + ".s");
    ASM::Assembler assembler(muncher, instr);
    assembler.assemble(asm_file);

    std::cout << "Applying optimizations...\n";

    for (int i = 1; i <= 1; i++) {
        Opt::optimize<Opt::OptimizationType::DEAD_COPY_REMOVAL>(muncher, instr, file_prefix);
        Opt::optimize<Opt::OptimizationType::JT_SEQ_UNCOND>(muncher, instr, file_prefix);
        Opt::optimize<Opt::OptimizationType::JT_COND_TO_UNCOND>(muncher, instr, file_prefix);
        Opt::optimize<Opt::OptimizationType::COALESCE>(muncher, instr, file_prefix);
    }
    return 0;
}