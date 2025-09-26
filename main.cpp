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

    std::ostringstream oss;
    oss << in.rdbuf();

    std::string file_content = oss.str();

    std::cout << file_content << "\n";

    Lexer::Lexer lexer(file_content);
    auto tokens = lexer.tokenize();

    for (auto &token : tokens) {
        std::cout << token << " ";
    }

    std::cout << "\n";

    Parser::Parser parser(tokens);
    std::cout << "Parsing and building AST...\n";
    auto ast = Grammar::Blocks::Program::match(parser);

    if (ast)
        ast->print(std::cout);
    // std::cout << ast << "\n";
    return 0;
}