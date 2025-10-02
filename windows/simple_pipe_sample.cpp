#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>

// Simple Named Pipe communication sample
// Parent: spawn 1 child process, send start/stop timer commands
// Child: run timer, send current time back to parent

// Command IDs
#define CMD_START_TIMER    1001
#define CMD_STOP_TIMER     1002
#define CMD_TIME_NOW       1003

// Simple message structure
struct SimpleMessage {
    DWORD command;
    DWORD dataSize;
    std::string data;
    
    SimpleMessage() : command(0), dataSize(0) {}
    SimpleMessage(DWORD cmd, const std::string& msg) : command(cmd), data(msg), dataSize(static_cast<DWORD>(msg.length())) {}
};

// Global variables
std::atomic<bool> g_timerRunning{false};
std::atomic<bool> g_childProcess{false};
HANDLE g_hPipe = INVALID_HANDLE_VALUE;
std::wstring g_pipeName = L"\\\\.\\pipe\\simple_timer_pipe";

// Utility functions
std::wstring utf8_to_wide(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring result(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size - 1);
    return result;
}

std::string wide_to_utf8(const std::wstring& str) {
    if (str.empty()) return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, &result[0], size - 1, nullptr, nullptr);
    return result;
}

void log(const std::wstring& message) {
    std::wcout << L"[" << (g_childProcess ? L"CHILD" : L"PARENT") << L"] " << message << std::endl;
}

// Named Pipe Server (Parent)
class SimplePipeServer {
private:
    HANDLE hPipe;
    std::wstring pipeName;
    std::atomic<bool> running{false};
    std::thread serverThread;

public:
    SimplePipeServer(const std::wstring& name) : pipeName(name), hPipe(INVALID_HANDLE_VALUE) {}
    
    ~SimplePipeServer() {
        Stop();
    }
    
    bool Start() {
        if (running.load()) return true;
        
        running.store(true);
        serverThread = std::thread(&SimplePipeServer::ServerLoop, this);
        return true;
    }
    
    void Stop() {
        if (!running.load()) return;
        
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
            if (!CreatePipe()) {
                log(L"Failed to create pipe");
                Sleep(1000);
                continue;
            }
            
            log(L"Server waiting for client connection");
            
            BOOL connected = ConnectNamedPipe(hPipe, NULL);
            if (!connected) {
                DWORD error = GetLastError();
                if (error == ERROR_PIPE_CONNECTED) {
                    log(L"Client already connected");
                } else {
                    log(L"ConnectNamedPipe failed: " + std::to_wstring(error));
                    CloseHandle(hPipe);
                    hPipe = INVALID_HANDLE_VALUE;
                    Sleep(1000);
                    continue;
                }
            }
            
            log(L"Client connected");
            
            // Handle messages from client
            while (running.load()) {
                SimpleMessage message;
                if (!ReadMessage(message)) {
                    log(L"Client disconnected");
                    break;
                }
                
                HandleMessage(message);
            }
            
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }
    }
    
    bool CreatePipe() {
        hPipe = CreateNamedPipeW(
            pipeName.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1,
            1024,
            1024,
            0,
            NULL
        );
        return hPipe != INVALID_HANDLE_VALUE;
    }
    
    bool ReadMessage(SimpleMessage& message) {
        DWORD bytesRead;
        if (!ReadFile(hPipe, &message.command, sizeof(DWORD), &bytesRead, NULL) || bytesRead != sizeof(DWORD)) {
            return false;
        }
        
        if (!ReadFile(hPipe, &message.dataSize, sizeof(DWORD), &bytesRead, NULL) || bytesRead != sizeof(DWORD)) {
            return false;
        }
        
        if (message.dataSize > 0) {
            message.data.resize(message.dataSize);
            if (!ReadFile(hPipe, &message.data[0], message.dataSize, &bytesRead, NULL) || bytesRead != message.dataSize) {
                return false;
            }
        }
        
        return true;
    }
    
    void HandleMessage(const SimpleMessage& message) {
        switch (message.command) {
        case CMD_TIME_NOW:
            log(L"Received time from child: " + utf8_to_wide(message.data));
            break;
        default:
            log(L"Unknown command: " + std::to_wstring(message.command));
            break;
        }
    }
};

// Named Pipe Client (Child)
class SimplePipeClient {
private:
    HANDLE hPipe;
    std::wstring pipeName;

public:
    SimplePipeClient(const std::wstring& name) : pipeName(name), hPipe(INVALID_HANDLE_VALUE) {}
    
    ~SimplePipeClient() {
        Disconnect();
    }
    
    bool Connect() {
        if (hPipe != INVALID_HANDLE_VALUE) return true;
        
        hPipe = CreateFileW(
            pipeName.c_str(),
            GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );
        
        if (hPipe != INVALID_HANDLE_VALUE) {
            log(L"Connected to parent pipe");
            return true;
        }
        
        log(L"Failed to connect to parent pipe: " + std::to_wstring(GetLastError()));
        return false;
    }
    
    void Disconnect() {
        if (hPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }
    }
    
    bool SendMessage(const SimpleMessage& message) {
        if (hPipe == INVALID_HANDLE_VALUE) return false;
        
        DWORD bytesWritten;
        if (!WriteFile(hPipe, &message.command, sizeof(DWORD), &bytesWritten, NULL) || bytesWritten != sizeof(DWORD)) {
            return false;
        }
        
        if (!WriteFile(hPipe, &message.dataSize, sizeof(DWORD), &bytesWritten, NULL) || bytesWritten != sizeof(DWORD)) {
            return false;
        }
        
        if (message.dataSize > 0) {
            if (!WriteFile(hPipe, message.data.c_str(), message.dataSize, &bytesWritten, NULL) || bytesWritten != message.dataSize) {
                return false;
            }
        }
        
        return true;
    }
};

// Timer function (Child)
void TimerFunction() {
    log(L"Timer started");
    
    while (g_timerRunning.load()) {
        // Get current time
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        
        std::string timeStr = std::to_string(time_t) + "." + std::to_string(ms.count());
        
        // Send time to parent
        SimplePipeClient client(g_pipeName);
        if (client.Connect()) {
            SimpleMessage message(CMD_TIME_NOW, timeStr);
            if (client.SendMessage(message)) {
                log(L"Sent time: " + utf8_to_wide(timeStr));
            } else {
                log(L"Failed to send time");
            }
        } else {
            log(L"Failed to connect to parent");
        }
        
        Sleep(1000); // Send every 1 second
    }
    
    log(L"Timer stopped");
}

// Child process main function
void ChildProcess() {
    g_childProcess = true;
    log(L"Child process started");
    
    SimplePipeClient client(g_pipeName);
    if (!client.Connect()) {
        log(L"Failed to connect to parent, exiting");
        return;
    }
    
    log(L"Child connected to parent, waiting for commands");
    
    // Simple message loop (in real implementation, this would be more sophisticated)
    while (true) {
        Sleep(100);
        // In a real implementation, you would receive commands from parent here
        // For this sample, we'll just run the timer
        if (!g_timerRunning.load()) {
            g_timerRunning.store(true);
            std::thread timerThread(TimerFunction);
            timerThread.detach();
        }
    }
}

// Parent process main function
void ParentProcess() {
    log(L"Parent process started");
    
    // Start Named Pipe server
    SimplePipeServer server(g_pipeName);
    if (!server.Start()) {
        log(L"Failed to start pipe server");
        return;
    }
    
    log(L"Pipe server started, waiting for child to connect");
    
    // Spawn child process
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(STARTUPINFO);
    
    wchar_t commandLine[] = L"simple_pipe_sample.exe child";
    if (CreateProcessW(NULL, commandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        log(L"Child process created with PID: " + std::to_wstring(pi.dwProcessId));
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        log(L"Failed to create child process: " + std::to_wstring(GetLastError()));
        return;
    }
    
    // Wait for child to connect and send some timer data
    log(L"Waiting for child to send timer data...");
    Sleep(5000); // Wait 5 seconds
    
    log(L"Parent process finished");
}

int main(int argc, char* argv[]) {
    if (argc > 1 && strcmp(argv[1], "child") == 0) {
        ChildProcess();
    } else {
        ParentProcess();
    }
    
    return 0;
}
