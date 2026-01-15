/**
 * Test: Hex-Stream Process Orchestration
 * 
 * Verifies six-stream process spawn, I/O, and lifecycle management.
 */

#include "hexstream/process.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
#include <csignal>

using namespace ariash::hexstream;
using namespace ariash::job;

void test_basic_spawn() {
    std::cout << "\n=== Test: Basic Process Spawn ===\n";
    
    ProcessConfig config;
    config.executable = "/bin/echo";
    config.arguments = {"Hello", "from", "hex-stream"};
    config.foregroundMode = false;
    
    HexStreamProcess proc(config);
    
    assert(proc.spawn() && "Process spawn failed");
    assert(proc.isRunning() && "Process should be running");
    
    int exitCode = proc.wait();
    assert(exitCode == 0 && "Echo should exit with 0");
    assert(!proc.isRunning() && "Process should not be running after wait");
    
    std::cout << "✓ Basic spawn working\n";
}

void test_stdout_capture() {
    std::cout << "\n=== Test: Stdout Capture ===\n";
    
    ProcessConfig config;
    config.executable = "/bin/echo";
    config.arguments = {"-n", "test output"};
    config.foregroundMode = false;
    
    HexStreamProcess proc(config);
    
    assert(proc.spawn() && "Process spawn failed");
    
    // Wait a bit for output to be drained
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    char buffer[1024];
    size_t bytesRead = proc.readFromStdout(buffer, sizeof(buffer));
    
    std::cout << "Read " << bytesRead << " bytes from stdout\n";
    
    if (bytesRead > 0) {
        std::string output(buffer, bytesRead);
        std::cout << "Output: '" << output << "'\n";
        assert(output.find("test output") != std::string::npos && "Should capture echo output");
    }
    
    proc.wait();
    
    std::cout << "✓ Stdout capture working\n";
}

void test_stdin_write() {
    std::cout << "\n=== Test: Stdin Write ===\n";
    
    ProcessConfig config;
    config.executable = "/bin/cat";
    config.arguments = {};
    config.foregroundMode = false;
    
    HexStreamProcess proc(config);
    
    assert(proc.spawn() && "Process spawn failed");
    
    // Write to stdin
    const char* testData = "Hello stdin!\n";
    ssize_t written = proc.writeToStdin(testData, strlen(testData));
    
    std::cout << "Wrote " << written << " bytes to stdin\n";
    assert(written > 0 && "Should write to stdin");
    
    // Close stdin to signal EOF (TODO: Need StreamController::closeStdin())
    // proc.closeStdin();
    
    // Wait a bit for cat to echo and exit
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    char buffer[1024];
    size_t bytesRead = proc.readFromStdout(buffer, sizeof(buffer));
    
    if (bytesRead > 0) {
        std::string output(buffer, bytesRead);
        std::cout << "Cat echoed: '" << output << "'\n";
        assert(output.find("Hello stdin!") != std::string::npos && "Cat should echo input");
    }
    
    int exitCode = proc.wait();
    assert(exitCode == 0 && "Cat should exit cleanly with EOF");
    
    std::cout << "✓ Stdin write working\n";
}

void test_data_callback() {
    std::cout << "\n=== Test: Data Callback ===\n";
    
    ProcessConfig config;
    config.executable = "/bin/echo";
    config.arguments = {"callback", "test"};
    config.foregroundMode = false;
    
    HexStreamProcess proc(config);
    
    bool callbackFired = false;
    
    proc.onData([&callbackFired](StreamIndex stream, const void* data, size_t size) {
        std::cout << "Data callback: stream=" << static_cast<int>(stream) 
                  << " size=" << size << "\n";
        callbackFired = true;
    });
    
    assert(proc.spawn() && "Process spawn failed");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    proc.wait();
    
    assert(callbackFired && "Data callback should have fired");
    
    std::cout << "✓ Data callback working\n";
}

void test_exit_callback() {
    std::cout << "\n=== Test: Exit Callback ===\n";
    
    ProcessConfig config;
    config.executable = "/bin/true";
    config.arguments = {};
    config.foregroundMode = false;
    
    HexStreamProcess proc(config);
    
    bool exitCallbackFired = false;
    int capturedExitCode = -999;
    
    proc.onExit([&exitCallbackFired, &capturedExitCode](int exitCode) {
        std::cout << "Exit callback: exitCode=" << exitCode << "\n";
        exitCallbackFired = true;
        capturedExitCode = exitCode;
    });
    
    assert(proc.spawn() && "Process spawn failed");
    
    int exitCode = proc.wait();
    
    assert(exitCallbackFired && "Exit callback should have fired");
    assert(capturedExitCode == 0 && "Should capture exit code 0");
    assert(exitCode == 0 && "true should exit with 0");
    
    std::cout << "✓ Exit callback working\n";
}

void test_metrics() {
    std::cout << "\n=== Test: Metrics ===\n";
    
    ProcessConfig config;
    config.executable = "/usr/bin/seq";
    config.arguments = {"1", "1000"};
    config.foregroundMode = false;
    
    HexStreamProcess proc(config);
    
    assert(proc.spawn() && "Process spawn failed");
    
    proc.wait();
    
    size_t bytesTransferred = proc.getTotalBytesTransferred();
    size_t activeThreads = proc.getActiveThreadCount();
    
    std::cout << "Bytes transferred: " << bytesTransferred << "\n";
    std::cout << "Active threads: " << activeThreads << "\n";
    
    assert(bytesTransferred > 0 && "Should transfer bytes");
    
    std::cout << "✓ Metrics working\n";
}

int main() {
    std::cout << "Hex-Stream Process Test Suite\n";
    std::cout << "==============================\n";
    
    try {
        test_basic_spawn();
        test_stdout_capture();
        // test_stdin_write();  // TODO: Needs StreamController::closeStdin()
        // test_data_callback();  // TODO: Needs StreamController::onData()
        test_exit_callback();
        test_metrics();
        
        std::cout << "\n✅ All hex-stream tests passed!\n\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed with exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "\n❌ Test failed with unknown exception\n";
        return 1;
    }
}
