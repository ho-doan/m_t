#include <flutter/dart_project.h>
#include <flutter/flutter_view_controller.h>
#include <windows.h>

#include "flutter_window.h"
#include "utils.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <string>

#include "local_push_connectivity/local_push_connectivity_plugin_c_api.h"

// IPC Communication between Parent and Child
std::atomic<bool> g_isChild{false};
std::atomic<int> g_currentNumber{0};
std::atomic<bool> g_numberReady{false};

void log(const std::wstring& msg) {
    std::wcout << L"[" << (g_isChild ? L"CHILD" : L"PARENT") << L"] " << msg << std::endl;
}

// Simple message structure for IPC
struct IPCMessage {
    DWORD cmd;
    DWORD data;
    
    IPCMessage() : cmd(0), data(0) {}
    IPCMessage(DWORD c, DWORD d) : cmd(c), data(d) {}
};

// Command IDs
#define IPC_CMD_SEND_NUMBER    2001
#define IPC_CMD_RECEIVE_RESULT 2002
#define IPC_CMD_PING           2003
#define IPC_CMD_PONG            2004

// Parent: Named Pipe Server
class ParentIPCServer {
    HANDLE hPipe;
    std::atomic<bool> running{false};
    std::thread serverThread;

public:
    ParentIPCServer() : hPipe(INVALID_HANDLE_VALUE) {}
    
    ~ParentIPCServer() { Stop(); }
    
    bool Start() {
        if (running.load()) return true;
        
        running.store(true);
        serverThread = std::thread(&ParentIPCServer::ServerLoop, this);
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
                L"\\\\.\\pipe\\flutter_ipc_pipe",
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                1, 1024, 1024, 0, NULL
            );
            
            if (hPipe == INVALID_HANDLE_VALUE) {
                log(L"Failed to create pipe");
                Sleep(1000);
                continue;
            }
            
            log(L"Parent server waiting for child...");
            
            // Wait for connection
            if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
                log(L"Child connected to parent!");
                
                // Handle messages
                while (running.load()) {
                    IPCMessage msg;
                    DWORD bytesRead;
                    
                    if (!ReadFile(hPipe, &msg.cmd, sizeof(DWORD), &bytesRead, NULL) || bytesRead != sizeof(DWORD)) {
                        break;
                    }
                    
                    if (!ReadFile(hPipe, &msg.data, sizeof(DWORD), &bytesRead, NULL) || bytesRead != sizeof(DWORD)) {
                        break;
                    }
                    
                    HandleMessage(msg);
                }
                
                log(L"Child disconnected from parent");
            }
            
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }
    }
    
    void HandleMessage(const IPCMessage& msg) {
        switch (msg.cmd) {
        case IPC_CMD_RECEIVE_RESULT:
            g_currentNumber.store(msg.data);
            g_numberReady.store(true);
            log(L"Received result from child: " + std::to_wstring(msg.data));
            break;
        case IPC_CMD_PONG:
            log(L"Received PONG from child");
            break;
        default:
            log(L"Unknown command from child: " + std::to_wstring(msg.cmd));
            break;
        }
    }
};

// Child: Named Pipe Client
class ChildIPCClient {
    HANDLE hPipe;
    
public:
    ChildIPCClient() : hPipe(INVALID_HANDLE_VALUE) {}
    
    ~ChildIPCClient() { Disconnect(); }
    
    bool Connect() {
        if (hPipe != INVALID_HANDLE_VALUE) return true;
        
        hPipe = CreateFileW(L"\\\\.\\pipe\\flutter_ipc_pipe", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        
        if (hPipe != INVALID_HANDLE_VALUE) {
            log(L"Child connected to parent pipe");
            return true;
        }
        
        log(L"Child failed to connect: " + std::to_wstring(GetLastError()));
        return false;
    }
    
    void Disconnect() {
        if (hPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }
    }
    
    bool SendMessage(const IPCMessage& msg) {
        if (hPipe == INVALID_HANDLE_VALUE) return false;
        
        DWORD bytesWritten;
        if (!WriteFile(hPipe, &msg.cmd, sizeof(DWORD), &bytesWritten, NULL) || bytesWritten != sizeof(DWORD)) {
            return false;
        }
        
        if (!WriteFile(hPipe, &msg.data, sizeof(DWORD), &bytesWritten, NULL) || bytesWritten != sizeof(DWORD)) {
            return false;
        }
        
        return true;
    }
};

// Child process main function
void RunChild() {
    g_isChild = true;
    log(L"Child process started (No UI)");
    
    ChildIPCClient client;
    if (!client.Connect()) {
        log(L"Child failed to connect to parent");
        return;
    }
    
    log(L"Child ready to receive numbers from parent");
    
    // Child process loop - listen for numbers from parent
    while (true) {
        // In a real implementation, child would listen for messages from parent
        // For demo, we'll simulate receiving numbers and sending back +1
        Sleep(2000); // Wait 2 seconds between operations
        
        // Simulate receiving a number (in real implementation, this would come from parent)
        int receivedNumber = rand() % 100; // Random number for demo
        int result = receivedNumber + 1;
        
        log(L"Child received number: " + std::to_wstring(receivedNumber));
        log(L"Child sending result: " + std::to_wstring(result));
        
        // Send result back to parent
        IPCMessage msg(IPC_CMD_RECEIVE_RESULT, result);
        if (client.SendMessage(msg)) {
            log(L"Child sent result to parent: " + std::to_wstring(result));
        } else {
            log(L"Child failed to send result to parent");
        }
    }
    
    log(L"Child process finished");
}

// Parent process main function
void RunParent() {
    log(L"Parent process started");
    
    // Start IPC server
    ParentIPCServer server;
    if (!server.Start()) {
        log(L"Failed to start parent IPC server");
        return;
    }
    
    log(L"Parent IPC server started, waiting for child...");
    
    // Spawn child process
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(STARTUPINFO);
    
    wchar_t cmdLine[] = L"local_push_connectivity_example.exe child";
    if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        log(L"Child created with PID: " + std::to_wstring(pi.dwProcessId));
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        log(L"Failed to create child: " + std::to_wstring(GetLastError()));
        return;
    }
    
    // Wait for IPC communication
    log(L"Waiting for IPC communication...");
    Sleep(10000); // Wait 10 seconds for demo
    
    log(L"Parent finished");
    server.Stop();
}

// Flutter integration functions
extern "C" {
    __declspec(dllexport) bool FlutterIPC_Initialize() {
        log(L"Flutter: Initializing IPC system");
        return true;
    }
    
    __declspec(dllexport) bool FlutterIPC_Start() {
        log(L"Flutter: Starting IPC system");
        RunParent();
        return true;
    }
    
    __declspec(dllexport) void FlutterIPC_Stop() {
        log(L"Flutter: Stopping IPC system");
    }
    
    __declspec(dllexport) int FlutterIPC_GetCurrentNumber() {
        return g_currentNumber.load();
    }
    
    __declspec(dllexport) bool FlutterIPC_IsNumberReady() {
        return g_numberReady.load();
    }
    
    __declspec(dllexport) void FlutterIPC_ResetNumberReady() {
        g_numberReady.store(false);
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prev,
    _In_ wchar_t* command_line, _In_ int show_command) {
    
    // Check if we're child process
    wchar_t* cmdLine = GetCommandLineW();
    if (wcsstr(cmdLine, L"child") != nullptr) {
        RunChild();
        return 0;
    }
    
    if (LocalPushConnectivityRegisterProcess(L"local_push_connectivity_example", command_line) == 0) {
        return 0;
    }
    std::wcout << command_line;
    
    // Attach to console when present (e.g., 'flutter run') or create a
    // new console when running with a debugger.
    if (!::AttachConsole(ATTACH_PARENT_PROCESS) && ::IsDebuggerPresent()) {
        CreateAndAttachConsole();
    }
    std::wcout << command_line;

    // Initialize COM, so that it is available for use in the library and/or
    // plugins.
    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    flutter::DartProject project(L"data");

    std::vector<std::string> command_line_arguments =
        GetCommandLineArguments();

    project.set_dart_entrypoint_arguments(std::move(command_line_arguments));

    FlutterWindow window(project);
    Win32Window::Point origin(10, 10);
    Win32Window::Size size(1280, 720);
    if (!window.Create(L"local_push_connectivity_example", origin, size)) {
        return EXIT_FAILURE;
    }
    window.SetQuitOnClose(true);

    // Start IPC system in background thread
    std::thread ipcThread([]() {
        Sleep(2000); // Wait 2 seconds for Flutter to initialize
        FlutterIPC_Start();
    });
    ipcThread.detach();

    ::MSG msg;
    while (::GetMessage(&msg, nullptr, 0, 0)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    // Stop IPC system before exit
    FlutterIPC_Stop();
    
    ::CoUninitialize();
    return EXIT_SUCCESS;
}
