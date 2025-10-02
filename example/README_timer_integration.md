# Flutter Timer Integration

## Mô tả
Tích hợp Named Pipe timer plugin vào Flutter app với C++ backend.

## Files đã tạo

### 1. C++ Integration
- `main.cpp` - Đã được cập nhật với timer functions
- `flutter_timer_plugin.dart` - Dart plugin để gọi C++ functions
- `timer_app.dart` - Flutter UI cho timer
- `main_with_timer.dart` - Main app với timer integration

### 2. C++ Functions đã thêm vào main.cpp
```cpp
// Timer functions
extern "C" {
    __declspec(dllexport) bool FlutterTimer_Initialize();
    __declspec(dllexport) bool FlutterTimer_Start();
    __declspec(dllexport) void FlutterTimer_Stop();
    __declspec(dllexport) bool FlutterTimer_IsRunning();
    __declspec(dllexport) const char* FlutterTimer_GetCurrentTime();
}
```

### 3. Dart Plugin
```dart
class FlutterTimerPlugin {
  Future<bool> initialize();
  Future<bool> startTimer();
  Future<void> stopTimer();
  bool isTimerRunning();
  String getCurrentTime();
}
```

## Cách sử dụng

### 1. Build Flutter app
```bash
cd example
flutter build windows
```

### 2. Chạy app
```bash
flutter run -d windows
```

### 3. Sử dụng timer
- App sẽ có 2 tabs: Main và Timer
- Tab Timer sẽ hiển thị timer status và current time
- Timer sẽ tự động start sau 2 giây khi app khởi động

## Expected Output

### Console Output
```
[TIMER] Flutter: Initializing timer plugin
[TIMER] Flutter: Starting timer
[TIMER] Timer started
[TIMER] Current time: 1703123456.123
[TIMER] Current time: 1703123457.124
[TIMER] Current time: 1703123458.125
```

### Flutter UI
- Timer Status: Running/Stopped
- Current Time: Real-time timestamp
- Start/Stop buttons
- Background timer updates

## Features

### ✅ Timer Functions
- **Start Timer** - Bắt đầu timer trong background thread
- **Stop Timer** - Dừng timer
- **Get Current Time** - Lấy current time từ C++
- **Is Running** - Kiểm tra timer status

### ✅ Flutter Integration
- **FFI Integration** - Gọi C++ functions từ Dart
- **Real-time Updates** - Timer updates mỗi giây
- **UI Integration** - Flutter UI hiển thị timer data
- **Background Thread** - Timer chạy trong background

### ✅ C++ Backend
- **Atomic Operations** - Thread-safe timer control
- **Background Thread** - Timer chạy độc lập
- **Real-time Clock** - High-precision time tracking
- **Flutter Integration** - C-style exports cho Flutter

## Troubleshooting

### Lỗi "Failed to initialize"
- Kiểm tra executable path
- Đảm bảo app đã được build
- Kiểm tra architecture (x64 vs x86)

### Lỗi "Failed to start timer"
- Kiểm tra C++ functions
- Kiểm tra thread creation
- Kiểm tra memory allocation

### Lỗi "Failed to get current time"
- Kiểm tra timer status
- Kiểm tra string conversion
- Kiểm tra memory management

## Notes
- Timer plugin được tích hợp trực tiếp vào main.cpp
- Không cần external DLL
- Timer chạy trong background thread
- Real-time updates mỗi giây
- Có thể mở rộng thêm features:
  - Multiple timers
  - Timer callbacks
  - Timer settings
  - Error handling
  - Reconnection logic
