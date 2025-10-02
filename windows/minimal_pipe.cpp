#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

// Minimal Named Pipe Sample
// Parent: Creates pipe, spawns child, receives timer data
// Child: Connects to pipe, sends current time every second

std::wstring pipeName = L"\\\\.\\pipe\\minimal_pipe";

void log(const std::string& msg) {
    std::cout << "[" << (GetCommandLineW()[0] == 'c' ? "CHILD" : "PARENT") << "] " << msg << std::endl;
}

// Parent: Server
void RunParent() {
    log("Parent started");
    
    // Create pipe
    HANDLE hPipe = CreateNamedPipeW(
        pipeName.c_str(),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, 1024, 1024, 0, NULL
    );
    
    if (hPipe == INVALID_HANDLE_VALUE) {
        log("Failed to create pipe");
        return;
    }
    
    log("Pipe created, waiting for child...");
    
    // Wait for connection
    if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
        log("Child connected!");
        
        // Receive timer data
        for (int i = 0; i < 10; i++) {
            char buffer[256];
            DWORD bytesRead;
            
            if (ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL)) {
                buffer[bytesRead] = '\0';
                log("Received: " + std::string(buffer));
            } else {
                log("Failed to read from pipe");
                break;
            }
            
            Sleep(1000);
        }
        
        log("Parent finished");
    }
    
    CloseHandle(hPipe);
}

// Child: Client
void RunChild() {
    log("Child started");
    
    // Connect to pipe
    HANDLE hPipe = CreateFileW(pipeName.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    
    if (hPipe == INVALID_HANDLE_VALUE) {
        log("Failed to connect to pipe");
        return;
    }
    
    log("Connected to parent");
    
    // Send timer data
    for (int i = 0; i < 10; i++) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::string timeStr = "Time: " + std::to_string(time_t);
        
        DWORD bytesWritten;
        if (WriteFile(hPipe, timeStr.c_str(), timeStr.length(), &bytesWritten, NULL)) {
            log("Sent: " + timeStr);
        } else {
            log("Failed to write to pipe");
            break;
        }
        
        Sleep(1000);
    }
    
    log("Child finished");
    CloseHandle(hPipe);
}

int main() {
    // Check command line
    wchar_t* cmdLine = GetCommandLineW();
    if (wcsstr(cmdLine, L"child") != nullptr) {
        RunChild();
    } else {
        RunParent();
    }
    
    return 0;
}
