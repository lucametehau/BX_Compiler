#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer/lexer.h"
#include "ast/block.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Wrong usage! Call " << argv[0] << " [filename.bx]\n";
        return 1;
    }

    const std::string filename(argv[1]);
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
    auto ast = Grammar::Blocks::Program::match(parser);

    if (!ast) {
        throw std::runtime_error("Failed parsing file!");
        return 1;
    }

    ast->print(std::cout);

    MM::MM muncher;
    auto instr = ast->munch(muncher);

    std::string file_prefix = filename.substr(0, filename.find("."));
    muncher.jsonify(file_prefix + ".tac.json", instr);
    return 0;
}