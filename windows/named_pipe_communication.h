#pragma once

#include <windows.h>
#include <string>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <chrono>
#include <sstream>
#include <iomanip>

// Command IDs for Named Pipe communication
// Legacy commands (backward compatibility)
#define PIPE_CMD_UPDATE_SETTINGS   100
#define PIPE_CMD_UPDATE_REGISTER   200
#define PIPE_CMD_STOP_SERVICE      300
#define PIPE_CMD_RECONNECT         400
#define PIPE_CMD_PONG              777
#define PIPE_CMD_LOGOUT            99

// New protocol commands (matching diagram)
#define PROTOCOL_HELLO             1000
#define PROTOCOL_HELLO_ACK         1001
#define PROTOCOL_SET_URL           1002
#define PROTOCOL_ACK               1003
#define PROTOCOL_SOCKET_EVENT      1004
#define PROTOCOL_PING              1005
#define PROTOCOL_PONG              1006

// Protocol message structures (matching diagram)
struct HelloMessage {
    std::string type = "HELLO";
    int pid;
    std::string version = "1.0";
};

struct HelloAckMessage {
    std::string type = "HELLO_ACK";
    bool ok = true;
    int64_t server_time;
};

struct SetUrlMessage {
    std::string type = "SET_URL";
    std::string id;
    std::string url;
    std::string opts = "{}";
};

struct AckMessage {
    std::string type = "ACK";
    std::string id;
    std::string status;
};

struct SocketEventMessage {
    std::string type = "SOCKET_EVENT";
    std::string event;
    std::string payload = "";
    std::string id = "";
    std::string info = "{}";
};

struct PingMessage {
    std::string type = "PING";
    int64_t t;
};

struct PongMessage {
    std::string type = "PONG";
    int64_t t;
};

struct PipeMessage {
    DWORD command;
    DWORD dataSize;
    std::string data;
    
    PipeMessage() : command(0), dataSize(0) {}
    PipeMessage(DWORD cmd, const std::string& msg) : command(cmd), data(msg), dataSize(static_cast<DWORD>(msg.length())) {}
};

// JSON serialization functions
inline std::string toJson(const HelloMessage& msg) {
    std::ostringstream oss;
    oss << "{\"type\":\"" << msg.type << "\",\"pid\":" << msg.pid << ",\"ver\":\"" << msg.version << "\"}";
    return oss.str();
}

inline std::string toJson(const HelloAckMessage& msg) {
    std::ostringstream oss;
    oss << "{\"type\":\"" << msg.type << "\",\"ok\":" << (msg.ok ? "true" : "false") << ",\"server_time\":" << msg.server_time << "}";
    return oss.str();
}

inline std::string toJson(const SetUrlMessage& msg) {
    std::ostringstream oss;
    oss << "{\"type\":\"" << msg.type << "\",\"id\":\"" << msg.id << "\",\"url\":\"" << msg.url << "\",\"opts\":" << msg.opts << "}";
    return oss.str();
}

inline std::string toJson(const AckMessage& msg) {
    std::ostringstream oss;
    oss << "{\"type\":\"" << msg.type << "\",\"id\":\"" << msg.id << "\",\"status\":\"" << msg.status << "\"}";
    return oss.str();
}

inline std::string toJson(const SocketEventMessage& msg) {
    std::ostringstream oss;
    oss << "{\"type\":\"" << msg.type << "\",\"event\":\"" << msg.event << "\"";
    if (!msg.payload.empty()) oss << ",\"payload\":\"" << msg.payload << "\"";
    if (!msg.id.empty()) oss << ",\"id\":\"" << msg.id << "\"";
    if (!msg.info.empty()) oss << ",\"info\":" << msg.info;
    oss << "}";
    return oss.str();
}

inline std::string toJson(const PingMessage& msg) {
    std::ostringstream oss;
    oss << "{\"type\":\"" << msg.type << "\",\"t\":" << msg.t << "}";
    return oss.str();
}

inline std::string toJson(const PongMessage& msg) {
    std::ostringstream oss;
    oss << "{\"type\":\"" << msg.type << "\",\"t\":" << msg.t << "}";
    return oss.str();
}

// Utility functions
inline int64_t getCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

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
