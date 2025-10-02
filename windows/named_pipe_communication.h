#pragma once

#include <windows.h>
#include <string>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

// Command IDs for Named Pipe communication
#define PIPE_CMD_UPDATE_SETTINGS   100
#define PIPE_CMD_UPDATE_REGISTER   200
#define PIPE_CMD_STOP_SERVICE      300
#define PIPE_CMD_RECONNECT         400
#define PIPE_CMD_PONG              777
#define PIPE_CMD_LOGOUT            99

struct PipeMessage {
    DWORD command;
    DWORD dataSize;
    std::string data;
    
    PipeMessage() : command(0), dataSize(0) {}
    PipeMessage(DWORD cmd, const std::string& msg) : command(cmd), data(msg), dataSize(msg.length()) {}
};

class NamedPipeServer {
private:
    HANDLE hPipe;
    std::wstring pipeName;
    std::atomic<bool> running;
    std::thread serverThread;
    std::function<void(const PipeMessage&)> messageHandler;
    std::mutex pipeMutex;
    
public:
    NamedPipeServer(const std::wstring& name);
    ~NamedPipeServer();
    
    bool Start(std::function<void(const PipeMessage&)> handler);
    void Stop();
    bool IsRunning() const { return running.load(); }
    
private:
    void ServerLoop();
    bool CreatePipe();
    void ClosePipe();
};

class NamedPipeClient {
private:
    HANDLE hPipe;
    std::wstring pipeName;
    std::mutex pipeMutex;
    
public:
    NamedPipeClient(const std::wstring& name);
    ~NamedPipeClient();
    
    bool Connect();
    void Disconnect();
    bool SendMessage(const PipeMessage& message);
    bool IsConnected() const;
    
private:
    void ClosePipe();
};

// Utility functions
std::wstring GetPipeName(const std::wstring& appTitle);
PipeMessage CreatePipeMessage(DWORD command, const std::string& data);
bool WritePipeMessage(HANDLE hPipe, const PipeMessage& message);
bool ReadPipeMessage(HANDLE hPipe, PipeMessage& message);
