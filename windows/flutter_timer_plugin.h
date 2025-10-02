#pragma once

#include <windows.h>
#include <string>

// Flutter Timer Plugin
// Simple integration for Flutter app

class FlutterTimerPlugin {
public:
    static FlutterTimerPlugin& getInstance() {
        static FlutterTimerPlugin instance;
        return instance;
    }
    
    // Initialize the plugin
    bool initialize() {
        writeLog(L"FlutterTimerPlugin: Initializing");
        return true;
    }
    
    // Start timer with Named Pipe communication
    bool startTimer() {
        writeLog(L"FlutterTimerPlugin: Starting timer");
        
        // Start Named Pipe server
        if (!startPipeServer()) {
            writeLog(L"FlutterTimerPlugin: Failed to start pipe server");
            return false;
        }
        
        // Spawn child process
        if (!spawnChildProcess()) {
            writeLog(L"FlutterTimerPlugin: Failed to spawn child process");
            return false;
        }
        
        return true;
    }
    
    // Stop timer
    void stopTimer() {
        writeLog(L"FlutterTimerPlugin: Stopping timer");
        // Implementation to stop timer
    }
    
    // Check if timer is running
    bool isTimerRunning() {
        return m_timerRunning;
    }
    
    // Get current time from child
    std::string getCurrentTime() {
        return m_currentTime;
    }

private:
    FlutterTimerPlugin() : m_timerRunning(false) {}
    
    bool startPipeServer() {
        // Create Named Pipe
        m_hPipe = CreateNamedPipeW(
            L"\\\\.\\pipe\\flutter_timer_pipe",
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1, 1024, 1024, 0, NULL
        );
        
        if (m_hPipe == INVALID_HANDLE_VALUE) {
            writeLog(L"Failed to create pipe: " + std::to_wstring(GetLastError()));
            return false;
        }
        
        writeLog(L"Pipe server created successfully");
        return true;
    }
    
    bool spawnChildProcess() {
        STARTUPINFO si = {0};
        PROCESS_INFORMATION pi = {0};
        si.cb = sizeof(STARTUPINFO);
        
        wchar_t cmdLine[] = L"flutter_timer_plugin.exe child";
        if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            writeLog(L"Child process created with PID: " + std::to_wstring(pi.dwProcessId));
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return true;
        } else {
            writeLog(L"Failed to create child process: " + std::to_wstring(GetLastError()));
            return false;
        }
    }
    
    void writeLog(const std::wstring& message) {
        std::wcout << L"[FlutterTimerPlugin] " << message << std::endl;
    }
    
    HANDLE m_hPipe;
    bool m_timerRunning;
    std::string m_currentTime;
};

// C-style exports for Flutter
extern "C" {
    __declspec(dllexport) bool FlutterTimer_Initialize() {
        return FlutterTimerPlugin::getInstance().initialize();
    }
    
    __declspec(dllexport) bool FlutterTimer_Start() {
        return FlutterTimerPlugin::getInstance().startTimer();
    }
    
    __declspec(dllexport) void FlutterTimer_Stop() {
        FlutterTimerPlugin::getInstance().stopTimer();
    }
    
    __declspec(dllexport) bool FlutterTimer_IsRunning() {
        return FlutterTimerPlugin::getInstance().isTimerRunning();
    }
    
    __declspec(dllexport) const char* FlutterTimer_GetCurrentTime() {
        static std::string timeStr = FlutterTimerPlugin::getInstance().getCurrentTime();
        return timeStr.c_str();
    }
}
