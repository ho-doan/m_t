#include "flutter_timer_plugin.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

// Global variables
std::atomic<bool> g_timerRunning{false};
std::atomic<bool> g_isChild{false};
std::string g_currentTime;

void log(const std::wstring& msg) {
    std::wcout << L"[" << (g_isChild ? L"CHILD" : L"PARENT") << L"] " << msg << std::endl;
}

// Simple message structure
struct TimerMessage {
    DWORD cmd;
    DWORD size;
    char data[256];
    
    TimerMessage() : cmd(0), size(0) { memset(data, 0, sizeof(data)); }
    TimerMessage(DWORD c, const std::string& d) : cmd(c), size(0) {
        memset(data, 0, sizeof(data));
        if (d.length() < sizeof(data)) {
            strcpy_s(data, d.c_str());
            size = static_cast<DWORD>(d.length());
        }
    }
};

// Command IDs
#define CMD_START_TIMER    1001
#define CMD_STOP_TIMER     1002
#define CMD_TIME_NOW       1003
#define CMD_PING           1004
#define CMD_PONG           1005

// Parent: Named Pipe Server
class FlutterTimerServer {
    HANDLE hPipe;
    std::atomic<bool> running{false};
    std::thread serverThread;

public:
    FlutterTimerServer() : hPipe(INVALID_HANDLE_VALUE) {}
    
    ~FlutterTimerServer() { Stop(); }
    
    bool Start() {
        if (running.load()) return true;
        
        running.store(true);
        serverThread = std::thread(&FlutterTimerServer::ServerLoop, this);
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
                L"\\\\.\\pipe\\flutter_timer_pipe",
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                1, 1024, 1024, 0, NULL
            );
            
            if (hPipe == INVALID_HANDLE_VALUE) {
                log(L"Failed to create pipe");
                Sleep(1000);
                continue;
            }
            
            log(L"Server waiting for child...");
            
            // Wait for connection
            if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
                log(L"Child connected!");
                
                // Handle messages
                while (running.load()) {
                    TimerMessage msg;
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
                    
                    HandleMessage(msg);
                }
                
                log(L"Child disconnected");
            }
            
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }
    }
    
    void HandleMessage(const TimerMessage& msg) {
        switch (msg.cmd) {
        case CMD_TIME_NOW:
            g_currentTime = std::string(msg.data, msg.size);
            log(L"Received time: " + std::wstring(msg.data, msg.data + msg.size));
            break;
        case CMD_PONG:
            log(L"Received PONG from child");
            break;
        default:
            log(L"Unknown command: " + std::to_wstring(msg.cmd));
            break;
        }
    }
};

// Child: Named Pipe Client
class FlutterTimerClient {
    HANDLE hPipe;
    
public:
    FlutterTimerClient() : hPipe(INVALID_HANDLE_VALUE) {}
    
    ~FlutterTimerClient() { Disconnect(); }
    
    bool Connect() {
        if (hPipe != INVALID_HANDLE_VALUE) return true;
        
        hPipe = CreateFileW(L"\\\\.\\pipe\\flutter_timer_pipe", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        
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
    
    bool SendMessage(const TimerMessage& msg) {
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
    FlutterTimerClient client;
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
        TimerMessage msg(CMD_TIME_NOW, timeStr);
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
    FlutterTimerServer server;
    if (!server.Start()) {
        log(L"Failed to start server");
        return;
    }
    
    log(L"Server started, waiting for child...");
    
    // Spawn child
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(STARTUPINFO);
    
    wchar_t cmdLine[] = L"flutter_timer_plugin.exe child";
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

// Flutter integration functions
extern "C" {
    __declspec(dllexport) bool FlutterTimer_Initialize() {
        log(L"Flutter: Initializing timer plugin");
        return true;
    }
    
    __declspec(dllexport) bool FlutterTimer_Start() {
        log(L"Flutter: Starting timer");
        RunParent();
        return true;
    }
    
    __declspec(dllexport) void FlutterTimer_Stop() {
        log(L"Flutter: Stopping timer");
        g_timerRunning.store(false);
    }
    
    __declspec(dllexport) bool FlutterTimer_IsRunning() {
        return g_timerRunning.load();
    }
    
    __declspec(dllexport) const char* FlutterTimer_GetCurrentTime() {
        static std::string timeStr = g_currentTime;
        return timeStr.c_str();
    }
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
