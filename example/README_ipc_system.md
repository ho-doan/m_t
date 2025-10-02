# Flutter IPC System - 2 Process Communication

## Mô tả
Hệ thống IPC (Inter-Process Communication) giữa 2 process:
- **Parent App**: Flutter app gửi số int
- **Child Process**: No UI, nhận số int và trả về int + 1

## Architecture

```
┌─────────────────────────────────┐
│  Parent App (Flutter UI)        │
│  ┌─────────────────────────────┐ │
│  │  Flutter IPC Plugin        │ │
│  │  - Send number             │ │
│  │  - Receive result          │ │
│  └─────────────────────────────┘ │
│  ┌─────────────────────────────┐ │
│  │  Named Pipe Server         │ │
│  │  - Listen for child        │ │
│  │  - Handle messages         │ │
│  └─────────────────────────────┘ │
└─────────────────────────────────┘
           │ Named Pipe IPC
           ▼
┌─────────────────────────────────┐
│  Child Process (No UI)          │
│  ┌─────────────────────────────┐ │
│  │  Named Pipe Client        │ │
│  │  - Connect to parent      │ │
│  │  - Send messages          │ │
│  └─────────────────────────────┘ │
│  ┌─────────────────────────────┐ │
│  │  Number Processor          │ │
│  │  - Receive number          │ │
│  │  - Add 1                   │ │
│  │  - Send result             │ │
│  └─────────────────────────────┘ │
└─────────────────────────────────┘
```

## Files Structure

### 1. C++ Backend (`main_ipc.cpp`)
```cpp
// Parent: Named Pipe Server
class ParentIPCServer {
    // Create Named Pipe
    // Listen for child connections
    // Handle IPC messages
};

// Child: Named Pipe Client  
class ChildIPCClient {
    // Connect to parent pipe
    // Send messages to parent
};

// Child Process (No UI)
void RunChild() {
    // Connect to parent
    // Listen for numbers
    // Process: number + 1
    // Send result back
}

// Parent Process (Flutter UI)
void RunParent() {
    // Start Named Pipe server
    // Spawn child process
    // Handle Flutter UI
}
```

### 2. Dart Plugin (`flutter_ipc_plugin.dart`)
```dart
class FlutterIPCPlugin {
  Future<bool> startIPC();           // Start IPC system
  Future<void> stopIPC();             // Stop IPC system
  int getCurrentNumber();             // Get number from child
  bool isNumberReady();               // Check if number ready
  void resetNumberReady();            // Reset ready flag
}
```

### 3. Flutter UI (`ipc_app.dart`)
```dart
class IPCApp extends StatefulWidget {
  // UI for IPC communication
  // Send numbers to child
  // Display received numbers
  // Start/Stop IPC system
}
```

## Communication Flow

### 1. Startup
```
Parent App → Start IPC System → Create Named Pipe Server
Parent App → Spawn Child Process → Child connects to pipe
Child Process → Connect to Parent → Ready for communication
```

### 2. Number Processing
```
Parent App → Send Number (int) → Named Pipe → Child Process
Child Process → Receive Number → Add 1 → Send Result
Child Process → Send Result (int + 1) → Named Pipe → Parent App
Parent App → Display Result → Flutter UI
```

### 3. Message Types
```cpp
#define IPC_CMD_SEND_NUMBER    2001  // Parent → Child
#define IPC_CMD_RECEIVE_RESULT 2002  // Child → Parent
#define IPC_CMD_PING           2003  // Ping
#define IPC_CMD_PONG            2004  // Pong
```

## UI Layout

```
┌─────────────────────────────────┐
│  IPC Status: [RUNNING/STOPPED]  │
├─────────────────────────────────┤
│                                 │
│  Sent Number: [123]             │
│  (Blue container)               │
│                                 │
│  Received Number: [124]         │
│  (Green container)              │
│                                 │
│  [Number Input] [Send]          │
│  (Grey container)               │
│                                 │
│  [Start IPC] [Stop IPC]         │
│                                 │
└─────────────────────────────────┘
```

## Features

### ✅ 2 Process System
- **Parent App**: Flutter UI với IPC controls
- **Child Process**: No UI, chỉ xử lý numbers
- **Named Pipe**: IPC communication channel
- **Parallel Execution**: 2 process chạy song song

### ✅ Number Processing
- **Send Number**: Parent gửi int đến child
- **Process Number**: Child nhận int, thêm 1
- **Return Result**: Child gửi (int + 1) về parent
- **Real-time Display**: Flutter UI hiển thị kết quả

### ✅ IPC Communication
- **Named Pipe**: Windows Named Pipe cho IPC
- **Message Protocol**: Structured message format
- **Error Handling**: Connection và message errors
- **Thread Safety**: Atomic operations

## Usage

### 1. Start IPC System
```dart
final plugin = FlutterIPCPlugin();
await plugin.initialize();
await plugin.startIPC();
```

### 2. Send Number
```dart
// User nhập number vào TextField
// Click Send button
// Number được gửi đến child process
```

### 3. Receive Result
```dart
// Child process nhận number
// Thêm 1: result = number + 1
// Gửi result về parent
// Flutter UI hiển thị result
```

## Expected Output

### Console Logs
```
[PARENT] Parent process started
[PARENT] Parent IPC server started, waiting for child...
[PARENT] Child created with PID: 1234
[PARENT] Child connected to parent!
[CHILD] Child process started (No UI)
[CHILD] Child connected to parent pipe
[CHILD] Child ready to receive numbers from parent
[CHILD] Child received number: 123
[CHILD] Child sending result: 124
[PARENT] Received result from child: 124
```

### Flutter UI
- **IPC Status**: RUNNING/STOPPED
- **Sent Number**: 123 (Blue container)
- **Received Number**: 124 (Green container)
- **Real-time Updates**: Numbers update automatically

## Process Flow

### Parent Process
1. **Start Flutter App** - UI initialization
2. **Create Named Pipe Server** - IPC channel
3. **Spawn Child Process** - Create child process
4. **Handle UI Events** - Send numbers, display results
5. **Receive Results** - Get processed numbers from child

### Child Process
1. **Start (No UI)** - Background process
2. **Connect to Parent** - Named Pipe connection
3. **Listen for Numbers** - Wait for parent messages
4. **Process Numbers** - Add 1 to received number
5. **Send Results** - Send (number + 1) back to parent

## Notes
- **Child Process**: No UI, chỉ xử lý numbers
- **Named Pipe**: Windows IPC mechanism
- **Real-time Communication**: Bidirectional message flow
- **Error Handling**: Connection và message errors
- **Thread Safety**: Atomic operations cho shared data
- **Process Management**: Parent spawns và manages child
- **Message Protocol**: Structured IPC message format
