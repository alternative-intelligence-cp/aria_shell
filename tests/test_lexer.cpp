/**
 * Lexer Test - Whitespace Insensitivity
 */

#include "parser/lexer.hpp"
#include <iostream>
#include <cassert>

using namespace ariash::parser;

void test_whitespace_insensitivity() {
    std::cout << "=== Test: Whitespace Insensitivity ===\n";
    
    // Same code, different formatting
    std::string minified = "if(x==1){y=2;}";
    std::string expanded = "if ( x == 1 ) { y = 2 ; }";
    
    ShellLexer lexer1(minified);
    ShellLexer lexer2(expanded);
    
    auto tokens1 = lexer1.tokenize();
    auto tokens2 = lexer2.tokenize();
    
    // Should produce identical token types
    assert(tokens1.size() == tokens2.size() && "Token count should match");
    
    for (size_t i = 0; i < tokens1.size(); ++i) {
        assert(tokens1[i].type == tokens2[i].type && "Token types should match");
        std::cout << tokenTypeToString(tokens1[i].type) << " ";
    }
    
    std::cout << "\n✓ Whitespace insensitivity working\n";
}

void test_string_literals() {
    std::cout << "\n=== Test: String Literals ===\n";
    
    std::string code = R"(echo "Hello World")";
    
    ShellLexer lexer(code);
    auto tokens = lexer.tokenize();
    
    assert(tokens.size() >= 2);
    assert(tokens[0].type == TokenType::IDENTIFIER);
    assert(tokens[0].lexeme == "echo");
    assert(tokens[1].type == TokenType::STRING);
    assert(tokens[1].lexeme == "Hello World");
    
    std::cout << "String: \"" << tokens[1].lexeme << "\"\n";
    std::cout << "✓ String literals working\n";
}

void test_operators() {
    std::cout << "\n=== Test: Operators ===\n";
    
    std::string code = "x + 1 == 2 && y > 3";
    
    ShellLexer lexer(code);
    auto tokens = lexer.tokenize();
    
    std::cout << "Tokens (" << tokens.size() << "): ";
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << i << ":" << tokenTypeToString(tokens[i].type) << " ";
    }
    std::cout << "\n";
    
    // Verify operators
    assert(tokens[1].type == TokenType::PLUS);
    assert(tokens[3].type == TokenType::EQ);
    assert(tokens[5].type == TokenType::AND);
    assert(tokens[7].type == TokenType::GT);
    
    std::cout << "✓ Operators working\n";
}

void test_shell_operators() {
    std::cout << "\n=== Test: Shell Operators ===\n";
    
    std::string code = "ls -la | grep test > output.txt";
    
    ShellLexer lexer(code);
    auto tokens = lexer.tokenize();
    
    std::cout << "Tokens: ";
    for (const auto& tok : tokens) {
        std::cout << "[" << tokenTypeToString(tok.type);
        if (!tok.lexeme.empty()) {
            std::cout << ":" << tok.lexeme;
        }
        std::cout << "] ";
    }
    std::cout << "\n";
    
    // Find pipe and redirect
    bool foundPipe = false;
    bool foundRedirect = false;
    
    for (const auto& tok : tokens) {
        if (tok.type == TokenType::PIPE) foundPipe = true;
        if (tok.type == TokenType::GT) foundRedirect = true;  // > is GT token
    }
    
    assert(foundPipe && "Should find pipe operator");
    assert(foundRedirect && "Should find redirect operator");
    
    std::cout << "✓ Shell operators working\n";
}

void test_keywords() {
    std::cout << "\n=== Test: Keywords ===\n";
    
    std::string code = "if while for int8 tbb8";
    
    ShellLexer lexer(code);
    auto tokens = lexer.tokenize();
    
    assert(tokens[0].type == TokenType::KW_IF);
    assert(tokens[1].type == TokenType::KW_WHILE);
    assert(tokens[2].type == TokenType::KW_FOR);
    assert(tokens[3].type == TokenType::KW_INT8);
    assert(tokens[4].type == TokenType::KW_TBB8);
    
    std::cout << "✓ Keywords recognized\n";
}

int main() {
    std::cout << "Shell Lexer Test Suite\n";
    std::cout << "======================\n\n";
    
    try {
        test_whitespace_insensitivity();
        test_string_literals();
        test_operators();
        test_shell_operators();
        test_keywords();
        
        std::cout << "\n✅ All lexer tests passed!\n\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed: " << e.what() << "\n";
        return 1;
    }
}
