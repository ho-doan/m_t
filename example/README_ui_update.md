# Flutter Timer UI Update

## Mô tả
Đã cập nhật UI với navigation và 2 button start/stop cho timer app.

## Files đã cập nhật

### 1. `main.dart` - Main App với Navigation
```dart
class _MyAppState extends State<MyApp> {
  int _selectedIndex = 0;
  
  // Pages
  final List<Widget> _pages = [
    const MainPage(),
    const TimerApp(),
  ];

  // Bottom Navigation
  bottomNavigationBar: BottomNavigationBar(
    currentIndex: _selectedIndex,
    onTap: (index) => setState(() => _selectedIndex = index),
    items: [
      BottomNavigationBarItem(icon: Icon(Icons.home), label: 'Main'),
      BottomNavigationBarItem(icon: Icon(Icons.timer), label: 'Timer'),
    ],
  ),
}
```

### 2. `timer_app.dart` - Timer UI với 2 Buttons
```dart
// Start Button
ElevatedButton.icon(
  onPressed: _isRunning ? null : _startTimer,
  icon: Icon(Icons.play_arrow),
  label: Text('Start Timer'),
  style: ElevatedButton.styleFrom(
    backgroundColor: Colors.green,
    // ... styling
  ),
),

// Stop Button
ElevatedButton.icon(
  onPressed: _isRunning ? _stopTimer : null,
  icon: Icon(Icons.stop),
  label: Text('Stop Timer'),
  style: ElevatedButton.styleFrom(
    backgroundColor: Colors.red,
    // ... styling
  ),
),
```

## UI Features

### ✅ Navigation
- **Bottom Navigation** - 2 tabs: Main và Timer
- **Tab Switching** - Chuyển đổi giữa các tabs
- **Visual Indicators** - Icons và labels cho mỗi tab

### ✅ Timer UI
- **Timer Icon** - Circular icon với status colors
- **Status Display** - RUNNING/STOPPED với colors
- **Current Time** - Real-time timestamp display
- **Control Buttons** - Start/Stop buttons với icons
- **Status Message** - Background running indicator

### ✅ Styling
- **Gradient Background** - Blue gradient từ top xuống bottom
- **Rounded Containers** - Modern rounded corners
- **Color Coding** - Green cho running, Red cho stopped
- **Elevation** - Button shadows và depth
- **Responsive Layout** - Centered và padded content

## Cách sử dụng

### 1. Chạy app
```bash
cd example
flutter run -d windows
```

### 2. Navigation
- **Tab "Main"** - Local Push Connectivity main page
- **Tab "Timer"** - Timer app với start/stop controls

### 3. Timer Controls
- **Start Timer** - Bắt đầu timer (green button)
- **Stop Timer** - Dừng timer (red button)
- **Status Display** - Hiển thị RUNNING/STOPPED
- **Time Display** - Real-time current time

## Expected Output

### UI Layout
```
┌─────────────────────────────────┐
│  App Bar: Plugin example app    │
├─────────────────────────────────┤
│                                 │
│        🏠 Main    ⏰ Timer     │
│                                 │
│  [Timer Icon - Circular]        │
│                                 │
│  Timer Status: [RUNNING/STOPPED]│
│                                 │
│  Current Time: [timestamp]       │
│                                 │
│  [Start Timer] [Stop Timer]     │
│                                 │
│  [Status Message]               │
│                                 │
└─────────────────────────────────┘
```

### Button States
- **Start Button**: Enabled khi timer stopped, Disabled khi timer running
- **Stop Button**: Enabled khi timer running, Disabled khi timer stopped
- **Status Colors**: Green cho running, Red cho stopped
- **Time Updates**: Real-time updates mỗi giây

## Features

### ✅ Navigation
- **Bottom Navigation Bar** - 2 tabs với icons
- **Tab Switching** - Smooth transitions
- **Visual Feedback** - Selected tab highlighting

### ✅ Timer UI
- **Modern Design** - Gradient background, rounded corners
- **Status Indicators** - Color-coded status display
- **Control Buttons** - Start/Stop với icons
- **Real-time Updates** - Live time display
- **Responsive Layout** - Centered và padded

### ✅ User Experience
- **Intuitive Controls** - Clear start/stop buttons
- **Visual Feedback** - Status colors và animations
- **Real-time Data** - Live timer updates
- **Modern UI** - Material Design principles

## Notes
- UI được thiết kế theo Material Design
- Timer chạy trong background thread
- Real-time updates mỗi giây
- Responsive layout cho different screen sizes
- Color coding cho better UX
- Modern styling với gradients và shadows
