/**
 * Simple Executor Test - Just Variables
 */

#include "parser/lexer.hpp"
#include "parser/parser.hpp"
#include "executor/executor.hpp"
#include <iostream>

using namespace ariash;

int main() {
    std::cout << "=== Simple Executor Test ===\n\n";
    
    std::string code = R"(
        int8 x = 5;
        int8 y = 10;
        int8 sum = x + y;
    )";
    
    std::cout << "Code:\n" << code << "\n\n";
    
    parser::ShellLexer lexer(code);
    auto tokens = lexer.tokenize();
    
    parser::ShellParser parser(tokens);
    auto ast = parser.parseProgram();
    
    executor::Environment env;
    executor::Executor exec(env);
    exec.execute(*ast);
    
    std::cout << "Results:\n";
    std::cout << "x = " << executor::valueToString(env.get("x")) << "\n";
    std::cout << "y = " << executor::valueToString(env.get("y")) << "\n";
    std::cout << "sum = " << executor::valueToString(env.get("sum")) << "\n";
    
    std::cout << "\n✅ Executor working!\n";
    return 0;
}
