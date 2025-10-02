# FlutterTimer_UpdateAtTime Integration

## Mô tả
Đã thêm function `FlutterTimer_UpdateAtTime` để hiển thị formatted time trong Flutter app.

## Files đã cập nhật

### 1. `main.cpp` - C++ Backend
```cpp
// Thêm global variable
std::string g_updateTime;

// Cập nhật TimerLoop để tạo formatted time
void TimerLoop() {
    while (g_timerRunning.load()) {
        // Current time (timestamp)
        g_currentTime = std::to_string(time_t) + "." + std::to_string(ms.count());
        
        // Update time (formatted)
        auto time_point = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_point);
        char buffer[64];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
        g_updateTime = std::string(buffer) + "." + std::to_string(ms.count());
    }
}

// Thêm export function
extern "C" {
    __declspec(dllexport) const char* FlutterTimer_UpdateAtTime() {
        return g_updateTime.c_str();
    }
}
```

### 2. `flutter_timer_plugin.dart` - Dart Plugin
```dart
class FlutterTimerPlugin {
  // Get update at time
  String getUpdateAtTime() {
    final getUpdateAtTime = _dylib
        .lookupFunction<Pointer<Utf8> Function(), Pointer<Utf8> Function()>(
          'FlutterTimer_UpdateAtTime',
        );
    
    final timePtr = getUpdateAtTime();
    return timePtr.toDartString();
  }
}
```

### 3. `timer_app.dart` - Flutter UI
```dart
class _TimerAppState extends State<TimerApp> {
  String _currentTime = '';
  String _updateTime = '';
  
  // Polling cả hai time values
  _timer = Timer.periodic(Duration(seconds: 1), (timer) {
    final currentTime = _plugin.getCurrentTime();
    final updateTime = _plugin.getUpdateAtTime();
    setState(() {
      _currentTime = currentTime;
      _updateTime = updateTime;
    });
  });
}
```

## Time Formats

### Current Time (g_currentTime)
- **Format**: Unix timestamp + milliseconds
- **Example**: `1703123456.123`
- **Usage**: Raw timestamp for calculations
- **Display**: Blue container

### Update Time (g_updateTime)
- **Format**: Human-readable date + time + milliseconds
- **Example**: `2023-12-21 14:30:56.123`
- **Usage**: User-friendly display
- **Display**: Green container

## UI Layout

```
┌─────────────────────────────────┐
│  Timer Status: [RUNNING/STOPPED]│
├─────────────────────────────────┤
│                                 │
│  Current Time:                  │
│  [1703123456.123]               │
│  (Blue container)               │
│                                 │
│  Update Time:                   │
│  [2023-12-21 14:30:56.123]      │
│  (Green container)              │
│                                 │
│  [Start Timer] [Stop Timer]     │
│                                 │
└─────────────────────────────────┘
```

## Features

### ✅ Dual Time Display
- **Current Time** - Unix timestamp format
- **Update Time** - Human-readable format
- **Real-time Updates** - Both update every second
- **Color Coding** - Blue cho current, Green cho update

### ✅ C++ Backend
- **Timestamp Generation** - Unix timestamp + milliseconds
- **Formatted Time** - strftime formatting
- **Export Functions** - C-style exports cho Flutter
- **Thread Safety** - Atomic operations

### ✅ Flutter Integration
- **FFI Functions** - Gọi C++ từ Dart
- **Real-time Polling** - Timer updates mỗi giây
- **UI Display** - Dual time containers
- **State Management** - setState cho updates

## Expected Output

### Console Logs
```
[TIMER] Timer started
[TIMER] Current time: 1703123456.123
[TIMER] Update time: 2023-12-21 14:30:56.123
[TIMER] Current time: 1703123457.124
[TIMER] Update time: 2023-12-21 14:30:57.124
```

### Flutter UI
- **Current Time**: `1703123456.123` (Blue container)
- **Update Time**: `2023-12-21 14:30:56.123` (Green container)
- **Real-time Updates**: Both values update every second
- **Status Display**: RUNNING/STOPPED indicator

## Usage

### 1. Start Timer
```dart
await _plugin.startTimer();
// Both current time and update time will start updating
```

### 2. Get Times
```dart
String currentTime = _plugin.getCurrentTime();    // "1703123456.123"
String updateTime = _plugin.getUpdateAtTime();    // "2023-12-21 14:30:56.123"
```

### 3. Stop Timer
```dart
await _plugin.stopTimer();
// Both time values will be cleared
```

## Notes
- **Current Time**: Raw timestamp cho calculations
- **Update Time**: Formatted time cho user display
- **Real-time Updates**: Both update every second
- **Color Coding**: Blue cho current, Green cho update
- **Thread Safety**: Atomic operations trong C++
- **FFI Integration**: C-style exports cho Flutter
