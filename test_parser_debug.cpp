#include "parser/parser.hpp"
#include "lexer/lexer.hpp"
#include <iostream>

int main() {
    std::string input = "int8 x = 5;";
    
    std::cout << "Input: '" << input << "'\n";
    
    // Lex
    Lexer lexer(input);
    auto tokens = lexer.tokenize();
    
    std::cout << "Tokens:\n";
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << "  [" << i << "] " << tokens[i].toString() << "\n";
    }
    
    // Parse
    std::cout << "\nParsing...\n";
    ShellParser parser(tokens);
    
    try {
        auto ast = parser.parseProgram();
        std::cout << "SUCCESS! Parsed " << ast->statements.size() << " statements\n";
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
