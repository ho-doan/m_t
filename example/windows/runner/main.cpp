#include <flutter/dart_project.h>
#include <flutter/flutter_view_controller.h>
#include <windows.h>

#include "flutter_window.h"
#include "utils.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

#include "local_push_connectivity/local_push_connectivity_plugin_c_api.h"

// Simple Timer Integration
std::atomic<bool> g_timerRunning{false};
std::string g_currentTime;
std::string g_updateTime;
std::string g_setTime;

void log(const std::wstring& msg) {
    std::wcout << L"[TIMER] " << msg << std::endl;
}

// Timer function that runs in background
void TimerLoop() {
    log(L"Timer started");
    
    while (g_timerRunning.load()) {
        // Get current time
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        
        g_currentTime = std::to_string(time_t) + "." + std::to_string(ms.count());
        
        // Get update time (formatted)
        auto time_point = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_point);
        char buffer[64];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
        g_updateTime = std::string(buffer) + "." + std::to_string(ms.count());
        
        log(L"Current time: " + std::wstring(g_currentTime.begin(), g_currentTime.end()));
        log(L"Update time: " + std::wstring(g_updateTime.begin(), g_updateTime.end()));
        
        Sleep(1000); // Update every second
    }
    
    log(L"Timer stopped");
}

// Flutter integration functions
extern "C" {
    __declspec(dllexport) bool FlutterTimer_Initialize() {
        log(L"Flutter: Initializing timer plugin");
        return true;
    }
    
    __declspec(dllexport) bool FlutterTimer_Start() {
        log(L"Flutter: Starting timer");
        g_timerRunning.store(true);
        
        // Start timer in background thread
        std::thread timerThread(TimerLoop);
        timerThread.detach();
        
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
        return g_currentTime.c_str();
    }
    
    __declspec(dllexport) const char* FlutterTimer_UpdateAtTime() {
        return g_updateTime.c_str();
    }
    
    __declspec(dllexport) void FlutterTimer_SetAtTime(const char* timeStr) {
        if (timeStr != nullptr) {
            g_setTime = std::string(timeStr);
            log(L"Set time: " + std::wstring(g_setTime.begin(), g_setTime.end()));
        }
    }
    
    __declspec(dllexport) const char* FlutterTimer_GetSetTime() {
        return g_setTime.c_str();
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prev,
    _In_ wchar_t* command_line, _In_ int show_command) {
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

    // Start timer in background thread
    std::thread timerThread([]() {
        Sleep(2000); // Wait 2 seconds for Flutter to initialize
        FlutterTimer_Start();
    });
    timerThread.detach();

    ::MSG msg;
    while (::GetMessage(&msg, nullptr, 0, 0)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    // Stop timer before exit
    FlutterTimer_Stop();
    
    ::CoUninitialize();
    return EXIT_SUCCESS;
}
