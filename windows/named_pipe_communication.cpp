#include "named_pipe_communication.h"
#include "utils.h"
#include <iostream>

// NamedPipeServer Implementation
NamedPipeServer::NamedPipeServer(const std::wstring& name) 
    : pipeName(name), running(false), hPipe(INVALID_HANDLE_VALUE) {
}

NamedPipeServer::~NamedPipeServer() {
    Stop();
}

bool NamedPipeServer::Start(std::function<void(const PipeMessage&)> handler) {
    if (running.load()) {
        return true;
    }
    
    messageHandler = handler;
    running.store(true);
    
    try {
        serverThread = std::thread(&NamedPipeServer::ServerLoop, this);
        return true;
    }
    catch (const std::exception& ex) {
        write_error(ex, 1001);
        running.store(false);
        return false;
    }
}

void NamedPipeServer::Stop() {
    if (!running.load()) {
        return;
    }
    
    running.store(false);
    
    // Close pipe to wake up server thread
    {
        std::lock_guard<std::mutex> lock(pipeMutex);
        if (hPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }
    }
    
    if (serverThread.joinable()) {
        serverThread.join();
    }
}

void NamedPipeServer::ServerLoop() {
    while (running.load()) {
        if (!CreatePipe()) {
            write_log(L"[NamedPipe] Failed to create pipe");
            Sleep(1000);
            continue;
        }
        
        write_log(L"[NamedPipe] Server waiting for client connection", L"");
        
        // Wait for client connection
        BOOL connected = ConnectNamedPipe(hPipe, NULL);
        if (!connected) {
            DWORD error = GetLastError();
            if (error == ERROR_PIPE_CONNECTED) {
                write_log(L"[NamedPipe] Client already connected");
            } else {
                write_log(L"[NamedPipe] ConnectNamedPipe failed: ", std::to_wstring(error).c_str());
                ClosePipe();
                Sleep(1000);
                continue;
            }
        }
        
        write_log(L"[NamedPipe] Client connected");
        
        // Handle messages from client
        while (running.load()) {
            PipeMessage message;
            if (!ReadPipeMessage(hPipe, message)) {
                DWORD error = GetLastError();
                if (error == ERROR_BROKEN_PIPE || error == ERROR_PIPE_NOT_CONNECTED) {
                    write_log(L"[NamedPipe] Client disconnected");
                    break;
                }
                write_log(L"[NamedPipe] ReadPipeMessage failed: ", std::to_wstring(error).c_str());
                break;
            }
            
            if (messageHandler) {
                try {
                    messageHandler(message);
                }
                catch (const std::exception& ex) {
                    write_error(ex, 1002);
                }
                catch (...) {
                    write_error();
                }
            }
        }
        
        ClosePipe();
    }
}

bool NamedPipeServer::CreatePipe() {
    std::lock_guard<std::mutex> lock(pipeMutex);
    
    hPipe = CreateNamedPipeW(
        pipeName.c_str(),
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, // Max instances
        4096, // Out buffer size
        4096, // In buffer size
        0, // Default timeout
        NULL // Security attributes
    );
    
    if (hPipe == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        write_log(L"[NamedPipe] CreateNamedPipe failed: ", std::to_wstring(error).c_str());
        return false;
    }
    
    return true;
}

void NamedPipeServer::ClosePipe() {
    std::lock_guard<std::mutex> lock(pipeMutex);
    if (hPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
    }
}

// NamedPipeClient Implementation
NamedPipeClient::NamedPipeClient(const std::wstring& name) 
    : pipeName(name), hPipe(INVALID_HANDLE_VALUE) {
}

NamedPipeClient::~NamedPipeClient() {
    Disconnect();
}

bool NamedPipeClient::Connect() {
    std::lock_guard<std::mutex> lock(pipeMutex);
    
    if (hPipe != INVALID_HANDLE_VALUE) {
        return true; // Already connected
    }
    
    // Wait for pipe to be available
    int retryCount = 0;
    const int maxRetries = 50; // 5 seconds total
    
    while (retryCount < maxRetries) {
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
            write_log(L"[NamedPipe] Client connected to server");
            return true;
        }
        
        DWORD error = GetLastError();
        if (error == ERROR_PIPE_BUSY) {
            write_log(L"[NamedPipe] Pipe busy, retrying...");
            Sleep(100);
            retryCount++;
        } else {
            write_log(L"[NamedPipe] CreateFile failed: ", std::to_wstring(error).c_str());
            break;
        }
    }
    
    return false;
}

void NamedPipeClient::Disconnect() {
    std::lock_guard<std::mutex> lock(pipeMutex);
    ClosePipe();
}

bool NamedPipeClient::SendMessage(const PipeMessage& message) {
    std::lock_guard<std::mutex> lock(pipeMutex);
    
    if (hPipe == INVALID_HANDLE_VALUE) {
        write_log(L"[NamedPipe] Not connected to server");
        return false;
    }
    
    return WritePipeMessage(hPipe, message);
}

bool NamedPipeClient::IsConnected() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(pipeMutex));
    return hPipe != INVALID_HANDLE_VALUE;
}

void NamedPipeClient::ClosePipe() {
    if (hPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
    }
}

// Utility functions
std::wstring GetPipeName(const std::wstring& appTitle) {
    return L"\\\\.\\pipe\\" + appTitle + L"_notification_pipe";
}

PipeMessage CreatePipeMessage(DWORD command, const std::string& data) {
    return PipeMessage(command, data);
}

bool WritePipeMessage(HANDLE hPipe, const PipeMessage& message) {
    DWORD bytesWritten;
    
    // Write command
    if (!WriteFile(hPipe, &message.command, sizeof(DWORD), &bytesWritten, NULL)) {
        return false;
    }
    
    // Write data size
    if (!WriteFile(hPipe, &message.dataSize, sizeof(DWORD), &bytesWritten, NULL)) {
        return false;
    }
    
    // Write data if any
    if (message.dataSize > 0) {
        if (!WriteFile(hPipe, message.data.c_str(), message.dataSize, &bytesWritten, NULL)) {
            return false;
        }
    }
    
    // Flush the pipe
    FlushFileBuffers(hPipe);
    return true;
}

bool ReadPipeMessage(HANDLE hPipe, PipeMessage& message) {
    DWORD bytesRead;
    
    // Read command
    if (!ReadFile(hPipe, &message.command, sizeof(DWORD), &bytesRead, NULL)) {
        return false;
    }
    
    // Read data size
    if (!ReadFile(hPipe, &message.dataSize, sizeof(DWORD), &bytesRead, NULL)) {
        return false;
    }
    
    // Read data if any
    if (message.dataSize > 0) {
        message.data.resize(message.dataSize);
        if (!ReadFile(hPipe, &message.data[0], message.dataSize, &bytesRead, NULL)) {
            return false;
        }
    }
    
    return true;
}
