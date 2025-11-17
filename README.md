# Math Puzzle Game - Multiplayer Server

Game giải đố toán học 4 người chơi với cơ chế thông tin bất đối xứng (Asymmetric Information).

## Mô tả Game

Đây là một trò chơi giải đố toán học 4 người chơi, tập trung vào sự phối hợp teamwork.

### Cơ chế chơi:
- Khi bắt đầu, cả nhóm sẽ nhận một phương trình toán học ngẫu nhiên (ví dụ: `P1 - P2 * P3 = P4`)
- Mỗi người chơi được gán một ma trận số 4x4
- **Điểm mấu chốt**: Bạn không thể thấy ma trận của chính mình, mà chỉ thấy được ma trận của 3 người còn lại
- Nhiệm vụ: Giao tiếp để giúp nhau tìm ra 4 con số chính xác (mỗi số từ một ma trận) để giải phương trình trong vòng **3 phút**
- Đây là game thử thách kỹ năng giao tiếp và logic, **không thể thắng nếu chơi một mình**

## Công nghệ

### Server Side (C)
- **Socket Programming**: TCP/IP sockets
- **I/O Multiplexing**: `select()` để quản lý nhiều client
- **Stream Processing**: Buffer với xử lý fragmentation
- **Protocol**: Text-based custom protocol
- **Platform**: Linux/Ubuntu

### Client Side (Qt C++) ✅
- **Qt Framework**: GUI và Signals/Slots
- **QTcpSocket**: Network communication
- **Event-driven**: readyRead signal processing
- **State Machine**: 5 screen management
- **Custom Widgets**: Matrix display components

## Cấu trúc dự án

```
Project/
├── server/               # C Server Implementation
│   ├── server.h          # Header definitions
│   ├── server.c          # Main server loop với select()
│   ├── auth.c            # Authentication & session management
│   ├── room.c            # Room management
│   ├── game.c            # Game logic & puzzle generation
│   ├── network.c         # PING/PONG & networking utilities
│   ├── Makefile          # Build configuration
│   └── users.txt         # User database (tạo tự động)
├── client/               # Qt Client Implementation ✅
│   ├── *.h, *.cpp        # Qt source files
│   ├── MathPuzzleClient.pro  # Qt project file
│   ├── CMakeLists.txt    # CMake build file
│   ├── build.bat/sh      # Build scripts
│   ├── README.md         # Client documentation
│   └── QUICKSTART.md     # Quick start guide
├── PROTOCOL.md           # Protocol documentation
└── README.md             # This file
```

## Build & Run

### Yêu cầu hệ thống
- Ubuntu 20.04+ (hoặc Linux distro khác)
- GCC compiler
- Make

### Biên dịch

```bash
cd server
make
```

### Chạy server

```bash
./game_server
```

Server sẽ lắng nghe trên port **8888**.

### Các lệnh make khác

```bash
make clean      # Xóa file build
make rebuild    # Clean và build lại
make run        # Build và chạy
```

## Tính năng đã implement

### Server Core ✅
- [x] I/O Multiplexing với `select()` (2 điểm)
- [x] Xử lý Stream với buffer (1 điểm)
- [x] Đăng ký/Đăng nhập & Quản lý Session (1 điểm)
- [x] PING/PONG & xử lý mất kết nối (2 điểm)
- [x] Quản lý phòng (Create/Join/Ready) (3 điểm)
- [x] Tạo puzzle & phân phối bất đối xứng (2 điểm)
- [x] Quản lý & đồng bộ thời gian (1 điểm)
- [x] Xác thực kết quả (Win/Lose) (3 điểm)

### Tính năng nâng cao ✅
- [x] Chat trong phòng (2 điểm)
- [x] PING/PONG tự động (1 điểm)

**Tổng điểm server: 15/15 + 3 nâng cao**

### Client Core ✅
- [x] Kết nối QTcpSocket (connected, disconnected) (2 điểm)
- [x] Xử lý readyRead event-driven (2 điểm)
- [x] Render 5 màn hình UI (Login, Lobby, Room, Game, Result) (3 điểm)
- [x] Sử dụng Qt Framework (Widgets, Layouts, Signals/Slots) (2 điểm)
- [x] State Machine quản lý chuyển màn hình (2 điểm)
- [x] Xử lý Input với Signals & Slots (2 điểm)
- [x] Parse & Update dữ liệu từ server (2 điểm)

### Client Bonus ✅
- [x] Chat system với UI đẹp
- [x] Auto-PONG response
- [x] Matrix cell selection UI
- [x] Timer với color indicators

**Tổng điểm client: 15/15 + bonus features**

## Protocol

Server sử dụng text-based protocol với delimiter `\n`. Xem chi tiết trong [PROTOCOL.md](PROTOCOL.md).

### Ví dụ:
- Client → Server: `LOGIN|username|password\n`
- Server → Client: `LOGIN_OK|username\n`
- Server → Client: `PING\n`
- Client → Server: `PONG\n`

## Testing với netcat/telnet

Bạn có thể test server bằng netcat:

```bash
# Kết nối tới server
nc localhost 8888

# Thử các lệnh
REGISTER|user1|pass123
LOGIN|user1|pass123
LIST_ROOMS
CREATE_ROOM|My Room
READY
CHAT|Hello everyone!
```

## Game Flow

1. **Đăng ký/Đăng nhập**
   - Client gửi `REGISTER` hoặc `LOGIN`
   - Server xác thực và chuyển client vào lobby

2. **Tạo/Tham gia phòng**
   - Client tạo phòng mới hoặc join phòng có sẵn
   - Tối đa 4 người/phòng

3. **Sẵn sàng**
   - Mỗi người bấm Ready
   - Khi cả 4 người ready → Game tự động bắt đầu

4. **Chơi game**
   - Server gửi phương trình và 3 ma trận (ẩn ma trận của bạn)
   - Players giao tiếp qua chat để tìm đáp án
   - Mỗi người submit tọa độ `[row, col]` từ ma trận ẩn của mình
   - Thời gian: 3 phút

5. **Kết thúc**
   - Khi cả 4 người submit → Server kiểm tra đáp án
   - Hoặc hết giờ → Server công bố thua
   - Hiển thị kết quả WIN/LOSE

## Kiến trúc Server

### I/O Multiplexing với select()
Server sử dụng `select()` để quản lý nhiều client trên một thread duy nhất:
- Lắng nghe socket mới
- Đọc dữ liệu từ client đã kết nối
- Timeout 1 giây để xử lý các tác vụ định kỳ (timer, ping)

### Stream Processing
Mỗi client có buffer riêng để xử lý:
- **Fragmentation**: Tin nhắn bị chia nhỏ qua nhiều `recv()`
- **Merging**: Nhiều tin nhắn gộp chung trong một `recv()`
- Delimiter `\n` để tách các message

### State Management
Client có các trạng thái:
- `CONNECTED` → `AUTHENTICATED` → `IN_LOBBY` → `IN_ROOM` → `READY` → `IN_GAME`

### PING/PONG Heartbeat
- Server gửi PING mỗi 10 giây
- Client phải trả lời PONG trong 30 giây
- Timeout → Server tự động disconnect và dọn dẹp

### Game Logic - Asymmetric Information
**Core mechanic của game:**
- Server tạo 4 ma trận 4x4
- Chọn ngẫu nhiên 1 số từ mỗi ma trận làm đáp án
- Tạo phương trình từ 4 số đó
- Gửi cho mỗi player: phương trình + 3 ma trận (ẩn ma trận của họ)

## Troubleshooting

### Port already in use
```bash
# Kiểm tra process đang dùng port 8888
sudo lsof -i :8888

# Kill process
kill -9 <PID>
```

### Permission denied
```bash
# Đảm bảo file có quyền execute
chmod +x game_server
```

## Development Roadmap

- [x] Server core implementation (C)
- [x] Qt Client implementation (C++)
- [x] Full protocol support
- [x] 5 UI screens with smooth transitions
- [x] Real-time chat system
- [ ] Advanced scoring system
- [ ] Server logging to file
- [ ] Multiple difficulty levels
- [ ] Replay system

## Quick Start

### Start Server
```bash
cd server
make run
# Server listens on port 8888
```

### Start Client
```bash
cd client
# Windows: build.bat
# Linux/Mac: ./build.sh
# Or open MathPuzzleClient.pro in Qt Creator
```

### Play Game
1. Connect to `localhost:8888`
2. Login or Register
3. Create/Join room (need 4 players)
4. Click Ready when 4 players join
5. Solve puzzle cooperatively via chat
6. Submit your answer within 3 minutes

See **[client/QUICKSTART.md](client/QUICKSTART.md)** for detailed instructions.

## Contributors

Dự án thực hành Lập trình mạng - Network Programming

## License

Educational project - Không dùng cho mục đích thương mại.

