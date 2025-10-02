#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>

// Simple Timer with Named Pipe Communication
// Parent: Controls timer start/stop
// Child: Runs timer and sends current time

std::wstring g_pipeName = L"\\\\.\\pipe\\timer_pipe";
std::atomic<bool> g_timerRunning{false};
std::atomic<bool> g_isChild{false};

void log(const std::wstring& msg) {
    std::wcout << L"[" << (g_isChild ? L"CHILD" : L"PARENT") << L"] " << msg << std::endl;
}

// Simple message structure
struct Message {
    DWORD cmd;
    DWORD size;
    char data[256];
    
    Message() : cmd(0), size(0) { memset(data, 0, sizeof(data)); }
    Message(DWORD c, const std::string& d) : cmd(c), size(0) {
        memset(data, 0, sizeof(data));
        if (d.length() < sizeof(data)) {
            strcpy_s(data, d.c_str());
            size = static_cast<DWORD>(d.length());
        }
    }
};

// Parent: Named Pipe Server
class ParentServer {
    HANDLE hPipe;
    std::atomic<bool> running{false};
    std::thread serverThread;

public:
    ParentServer() : hPipe(INVALID_HANDLE_VALUE) {}
    
    ~ParentServer() { Stop(); }
    
    bool Start() {
        if (running.load()) return true;
        
        running.store(true);
        serverThread = std::thread(&ParentServer::ServerLoop, this);
        return true;
    }
    
    void Stop() {
        running.store(false);
        if (hPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }
        if (serverThread.joinable()) {
            serverThread.join();
        }
    }

private:
    void ServerLoop() {
        while (running.load()) {
            // Create pipe
            hPipe = CreateNamedPipeW(
                g_pipeName.c_str(),
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                1, 1024, 1024, 0, NULL
            );
            
            if (hPipe == INVALID_HANDLE_VALUE) {
                log(L"Failed to create pipe");
                Sleep(1000);
                continue;
            }
            
            log(L"Waiting for child connection...");
            
            // Wait for connection
            if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
                log(L"Child connected!");
                
                // Handle messages
                while (running.load()) {
                    Message msg;
                    DWORD bytesRead;
                    
                    if (!ReadFile(hPipe, &msg.cmd, sizeof(DWORD), &bytesRead, NULL) || bytesRead != sizeof(DWORD)) {
                        break;
                    }
                    
                    if (!ReadFile(hPipe, &msg.size, sizeof(DWORD), &bytesRead, NULL) || bytesRead != sizeof(DWORD)) {
                        break;
                    }
                    
                    if (msg.size > 0 && msg.size < sizeof(msg.data)) {
                        if (!ReadFile(hPipe, msg.data, msg.size, &bytesRead, NULL) || bytesRead != msg.size) {
                            break;
                        }
                    }
                    
                    // Handle message
                    if (msg.cmd == 1001) { // TIME_NOW
                        log(L"Received time: " + std::wstring(msg.data, msg.data + msg.size));
                    }
                }
                
                log(L"Child disconnected");
            }
            
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }
    }
};

// Child: Named Pipe Client
class ChildClient {
    HANDLE hPipe;
    
public:
    ChildClient() : hPipe(INVALID_HANDLE_VALUE) {}
    
    ~ChildClient() { Disconnect(); }
    
    bool Connect() {
        if (hPipe != INVALID_HANDLE_VALUE) return true;
        
        hPipe = CreateFileW(g_pipeName.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        
        if (hPipe != INVALID_HANDLE_VALUE) {
            log(L"Connected to parent");
            return true;
        }
        
        log(L"Failed to connect: " + std::to_wstring(GetLastError()));
        return false;
    }
    
    void Disconnect() {
        if (hPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }
    }
    
    bool SendMessage(const Message& msg) {
        if (hPipe == INVALID_HANDLE_VALUE) return false;
        
        DWORD bytesWritten;
        if (!WriteFile(hPipe, &msg.cmd, sizeof(DWORD), &bytesWritten, NULL) || bytesWritten != sizeof(DWORD)) {
            return false;
        }
        
        if (!WriteFile(hPipe, &msg.size, sizeof(DWORD), &bytesWritten, NULL) || bytesWritten != sizeof(DWORD)) {
            return false;
        }
        
        if (msg.size > 0) {
            if (!WriteFile(hPipe, msg.data, msg.size, &bytesWritten, NULL) || bytesWritten != msg.size) {
                return false;
            }
        }
        
        return true;
    }
};

// Timer function
void TimerLoop() {
    ChildClient client;
    if (!client.Connect()) {
        log(L"Failed to connect to parent");
        return;
    }
    
    log(L"Timer started");
    
    while (g_timerRunning.load()) {
        // Get current time
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        
        std::string timeStr = std::to_string(time_t) + "." + std::to_string(ms.count());
        
        // Send to parent
        Message msg(1001, timeStr); // CMD_TIME_NOW
        if (client.SendMessage(msg)) {
            log(L"Sent time: " + std::wstring(timeStr.begin(), timeStr.end()));
        } else {
            log(L"Failed to send time");
            break;
        }
        
        Sleep(1000); // Send every second
    }
    
    log(L"Timer stopped");
    client.Disconnect();
}

// Child process
void RunChild() {
    g_isChild = true;
    log(L"Child process started");
    
    // Start timer
    g_timerRunning.store(true);
    TimerLoop();
    
    log(L"Child process finished");
}

// Parent process
void RunParent() {
    log(L"Parent process started");
    
    // Start server
    ParentServer server;
    if (!server.Start()) {
        log(L"Failed to start server");
        return;
    }
    
    log(L"Server started, waiting for child...");
    
    // Spawn child
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(STARTUPINFO);
    
    wchar_t cmdLine[] = L"simple_timer_pipe.exe child";
    if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        log(L"Child created with PID: " + std::to_wstring(pi.dwProcessId));
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        log(L"Failed to create child: " + std::to_wstring(GetLastError()));
        return;
    }
    
    // Wait for timer data
    log(L"Waiting for timer data...");
    Sleep(10000); // Wait 10 seconds
    
    log(L"Parent finished");
    server.Stop();
}

int main() {
    // Check if we're child process
    wchar_t* cmdLine = GetCommandLineW();
    if (wcsstr(cmdLine, L"child") != nullptr) {
        RunChild();
    } else {
        RunParent();
    }
    
    return 0;
}
