/**
 * Multi-Line Input Demo
 * 
 * Tests the modal FSM with raw terminal mode and protocol negotiation.
 */

#include "repl/terminal.hpp"
#include "repl/input_engine.hpp"
#include <iostream>
#include <cstdlib>

using namespace ariash::repl;

int main() {
    std::cout << "=== Aria Multi-Line Input Demo ===\n";
    std::cout << "Press Ctrl+Enter or Alt+Enter to submit code.\n";
    std::cout << "Press Ctrl+D on empty line to exit.\n";
    std::cout << "Press Ctrl+C to cancel current input.\n\n";
    
    PlatformTerminal terminal;
    
    // Enter raw mode
    if (!terminal.enterRawMode()) {
        std::cerr << "Failed to enter raw mode\n";
        return 1;
    }
    
    // Attempt protocol negotiation
    ProtocolLevel level = terminal.negotiateProtocol();
    std::cout << "Protocol level: ";
    switch (level) {
        case ProtocolLevel::KITTY_PROGRESSIVE:
            std::cout << "Kitty Progressive (✓ Ctrl+Enter supported)\n";
            break;
        case ProtocolLevel::XTERM_MODIFY_KEYS:
            std::cout << "XTerm modifyOtherKeys (✓ Ctrl+Enter supported)\n";
            break;
        case ProtocolLevel::LEGACY:
            std::cout << "Legacy (use Alt+Enter to submit)\n";
            break;
    }
    std::cout << "\n";
    
    // Create input engine
    InputEngine engine(terminal);
    
    // Set submission callback
    engine.onSubmission([](const std::string& code) {
        std::cout << "\n--- Code Submitted ---\n";
        std::cout << code << "\n";
        std::cout << "--- End ---\n\n";
    });
    
    // Set exit callback
    bool exited = false;
    engine.onExit([&exited]() {
        exited = true;
    });
    
    // Run event loop
    engine.run();
    
    // Restore terminal before exit
    terminal.restoreMode();
    
    if (exited) {
        std::cout << "Goodbye!\n";
    }
    
    return 0;
}
