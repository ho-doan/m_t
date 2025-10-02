# Simple Named Pipe Sample

## Mô tả
Sample đơn giản để test Named Pipe communication giữa parent và child process.

## Cách hoạt động
1. **Parent process**: Tạo Named Pipe server, spawn child process, nhận timer data
2. **Child process**: Kết nối đến parent pipe, gửi current time mỗi giây

## Files
- `minimal_pipe.cpp` - Code chính (đơn giản nhất)
- `simple_timer_pipe.cpp` - Version với timer class
- `simple_pipe_sample.cpp` - Version đầy đủ với message protocol
- `CMakeLists_simple.txt` - Build configuration

## Build và chạy

### Cách 1: Build với CMake
```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=CMakeLists_simple.txt
cmake --build .
```

### Cách 2: Build trực tiếp
```bash
g++ -std=c++17 minimal_pipe.cpp -o minimal_pipe.exe
```

### Chạy
```bash
# Chạy parent process
./minimal_pipe.exe

# Hoặc chạy child process trực tiếp
./minimal_pipe.exe child
```

## Expected Output
```
[PARENT] Parent started
[PARENT] Pipe created, waiting for child...
[PARENT] Child connected!
[PARENT] Received: Time: 1703123456
[PARENT] Received: Time: 1703123457
[PARENT] Received: Time: 1703123458
[PARENT] Parent finished

[CHILD] Child started
[CHILD] Connected to parent
[CHILD] Sent: Time: 1703123456
[CHILD] Sent: Time: 1703123457
[CHILD] Sent: Time: 1703123458
[CHILD] Child finished
```

## Các version khác nhau

### 1. minimal_pipe.cpp (Đơn giản nhất)
- Chỉ có basic Named Pipe communication
- Parent tạo pipe, child kết nối và gửi data
- Không có message protocol phức tạp

### 2. simple_timer_pipe.cpp (Có timer)
- Thêm timer functionality
- Child chạy timer và gửi current time
- Parent nhận và hiển thị time

### 3. simple_pipe_sample.cpp (Đầy đủ)
- Có message protocol với command IDs
- Có start/stop timer commands
- Có proper error handling

## Troubleshooting

### Lỗi "Pipe not found"
- Đảm bảo parent process chạy trước child
- Kiểm tra pipe name có đúng không
- Kiểm tra permissions

### Lỗi "Failed to create pipe"
- Pipe name đã được sử dụng
- Không có quyền tạo Named Pipe
- Thử đổi pipe name

### Lỗi "Failed to connect"
- Parent chưa tạo pipe
- Pipe name sai
- Network issues (nếu dùng remote pipe)

## Notes
- Sample này chỉ để test basic Named Pipe communication
- Không có error handling phức tạp
- Không có message framing
- Không có reconnection logic
- Chỉ để hiểu cách Named Pipe hoạt động
