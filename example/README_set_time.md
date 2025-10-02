# FlutterTimer_SetAtTime Integration

## Mô tả
Đã thêm function `FlutterTimer_SetAtTime` để cho phép user set custom time trong Flutter app.

## Files đã cập nhật

### 1. `main.cpp` - C++ Backend
```cpp
// Thêm global variable
std::string g_setTime;

// Thêm export functions
extern "C" {
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
```

### 2. `flutter_timer_plugin.dart` - Dart Plugin
```dart
class FlutterTimerPlugin {
  // Set at time
  Future<void> setAtTime(String timeStr) async {
    final setAtTime = _dylib.lookupFunction<
        Void Function(Pointer<Utf8>),
        void Function(Pointer<Utf8>)>('FlutterTimer_SetAtTime');

    final timePtr = timeStr.toNativeUtf8();
    setAtTime(timePtr);
    malloc.free(timePtr);
  }

  // Get set time
  String getSetTime() {
    final getSetTime = _dylib
        .lookupFunction<Pointer<Utf8> Function(), Pointer<Utf8> Function()>(
          'FlutterTimer_GetSetTime',
        );
    return getSetTime().toDartString();
  }
}
```

### 3. `timer_app.dart` - Flutter UI
```dart
class _TimerAppState extends State<TimerApp> {
  String _setTime = '';
  final TextEditingController _timeController = TextEditingController();
  
  // Set time function
  Future<void> _setTime() async {
    final timeStr = _timeController.text.trim();
    if (timeStr.isNotEmpty) {
      await _plugin.setAtTime(timeStr);
      _timeController.clear();
    }
  }
}
```

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
│  Set Time:                      │
│  [2023-12-21 15:00:00.000]      │
│  (Orange container)             │
│                                 │
│  [Time Input Field] [Set]      │
│  (Grey container)               │
│                                 │
│  [Start Timer] [Stop Timer]     │
│                                 │
└─────────────────────────────────┘
```

## Time Types

### Current Time (Blue)
- **Format**: Unix timestamp + milliseconds
- **Example**: `1703123456.123`
- **Source**: System clock
- **Usage**: Raw timestamp

### Update Time (Green)
- **Format**: Human-readable date + time + milliseconds
- **Example**: `2023-12-21 14:30:56.123`
- **Source**: System clock (formatted)
- **Usage**: User display

### Set Time (Orange)
- **Format**: User-defined time string
- **Example**: `2023-12-21 15:00:00.000`
- **Source**: User input
- **Usage**: Custom time setting

## Features

### ✅ Set Time Functionality
- **Input Field** - TextField để nhập time
- **Set Button** - Button để set time
- **Display** - Orange container hiển thị set time
- **Real-time Updates** - Set time được update mỗi giây

### ✅ C++ Backend
- **Set Function** - `FlutterTimer_SetAtTime(const char*)`
- **Get Function** - `FlutterTimer_GetSetTime()`
- **String Storage** - Global string variable
- **Logging** - Console logs cho debugging

### ✅ Flutter Integration
- **FFI Functions** - Gọi C++ từ Dart
- **Memory Management** - Proper malloc/free
- **Error Handling** - Try-catch blocks
- **UI Updates** - setState cho real-time updates

## Usage

### 1. Set Time
```dart
// User nhập time vào TextField
await _plugin.setAtTime("2023-12-21 15:00:00.000");
```

### 2. Get Set Time
```dart
String setTime = _plugin.getSetTime();
// Returns: "2023-12-21 15:00:00.000"
```

### 3. UI Interaction
- User nhập time vào TextField
- Click "Set" button
- Set time được hiển thị trong orange container
- Real-time updates mỗi giây

## Expected Output

### Console Logs
```
[TIMER] Set time: 2023-12-21 15:00:00.000
FlutterTimerPlugin: Set time: 2023-12-21 15:00:00.000
```

### Flutter UI
- **Current Time**: `1703123456.123` (Blue)
- **Update Time**: `2023-12-21 14:30:56.123` (Green)
- **Set Time**: `2023-12-21 15:00:00.000` (Orange)
- **Input Field**: TextField với hint text
- **Set Button**: Orange button để set time

## Input Format

### Supported Formats
- **ISO Format**: `2023-12-21 15:00:00`
- **Custom Format**: `2023-12-21 15:00:00.000`
- **Any String**: User có thể nhập bất kỳ string nào

### Examples
```
2023-12-21 15:00:00
2023-12-21 15:00:00.000
Custom time string
Any user-defined time
```

## Notes
- **Set Time**: User-defined time string
- **Real-time Updates**: Set time được update mỗi giây
- **Memory Management**: Proper malloc/free trong Dart
- **Error Handling**: Try-catch blocks cho stability
- **UI Integration**: TextField + Button + Display
- **Color Coding**: Orange cho set time
- **Input Validation**: Basic trim và empty check
