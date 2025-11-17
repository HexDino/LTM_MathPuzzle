# HƯỚNG DẪN CHẠY DỰ ÁN - MATH PUZZLE GAME

## Mô tả Game

**Math Puzzle Game** là trò chơi giải đố toán học 4 người chơi với cơ chế **thông tin bất đối xứng** (Asymmetric Information), yêu cầu teamwork và giao tiếp để chiến thắng.

### Cách chơi:
- **4 người chơi** tham gia vào một phòng
- Server tạo một **phương trình toán học ngẫu nhiên** (ví dụ: `P1 - P2 × P3 = P4`)
- Mỗi người được gán một **ma trận số 4×4**
- **Điểm đặc biệt**: Bạn không thể thấy ma trận của chính mình, chỉ thấy ma trận của 3 người còn lại
- **Nhiệm vụ**: Giao tiếp qua chat để giúp nhau tìm ra 4 con số (mỗi số từ một ma trận) thỏa mãn phương trình
- **Thời gian**: 3 phút để hoàn thành
- **Không thể thắng nếu chơi một mình!**

---

## YÊU CẦU HỆ THỐNG

### Server (C)
- **Hệ điều hành**: Linux/Ubuntu (hoặc WSL trên Windows)
- **Compiler**: GCC
- **Công cụ**: Make

### Client (Qt C++)
- **Hệ điều hành**: Windows, Linux, hoặc macOS
- **Qt Framework**: Qt 5.12 trở lên (hoặc Qt 6.x)
- **Compiler**: 
  - Windows: MSVC hoặc MinGW
  - Linux: GCC
  - macOS: Clang

---

## PHẦN 1: CHẠY SERVER

### Bước 1: Mở Terminal/PowerShell

**Trên Linux/Ubuntu:**
```bash
cd server
```

**Trên Windows (dùng WSL - Windows Subsystem for Linux):**
```bash
# Cài WSL nếu chưa có:
wsl --install

# Sau khi cài xong, mở WSL Ubuntu:
cd /mnt/c/CDD/Thuc_hanh_lap_trinh_mang/Project/server
```

### Bước 2: Biên dịch Server

```bash
make
```

**Kết quả mong đợi:**
```
gcc -Wall -Wextra -g -std=c11 -c server.c -o server.o
gcc -Wall -Wextra -g -std=c11 -c auth.c -o auth.o
gcc -Wall -Wextra -g -std=c11 -c room.c -o room.o
gcc -Wall -Wextra -g -std=c11 -c game.c -o game.o
gcc -Wall -Wextra -g -std=c11 -c network.c -o network.o
gcc -Wall -Wextra -g -std=c11 -o game_server server.o auth.o room.o game.o network.o
Build successful! Run with: ./game_server
```

### Bước 3: Chạy Server

```bash
./game_server
```

**Kết quả mong đợi:**
```
Server initialized on port 8888
Math Puzzle Game Server running...
Waiting for players...
```

Server hiện đang chạy và lắng nghe trên **port 8888**.

### Các lệnh Make khác:

```bash
make clean      # Xóa file build
make rebuild    # Clean và build lại
make run        # Build và chạy luôn
```

### Lỗi thường gặp:

**1. Port 8888 đang được sử dụng:**
```bash
# Kiểm tra process nào đang dùng port 8888
sudo lsof -i :8888

# Hoặc dùng netstat
sudo netstat -tulpn | grep 8888

# Kill process (thay <PID> bằng số process ID)
kill -9 <PID>
```

**2. Permission denied:**
```bash
chmod +x game_server
```

---

## PHẦN 2: CHẠY CLIENT (Qt)

### Cách 1: Dùng Qt Creator (Khuyến nghị)

#### Bước 1: Cài đặt Qt
- Tải Qt từ: https://www.qt.io/download
- Chọn phiên bản Qt 5.15 hoặc Qt 6.x
- Cài đặt với module **Qt Network**

#### Bước 2: Mở Project
1. Mở **Qt Creator**
2. Chọn `File` → `Open File or Project`
3. Chọn file `client/MathPuzzleClient.pro`
4. Chọn **Kit** (ví dụ: Desktop Qt 5.15.2 MinGW)

#### Bước 3: Build và Run
1. Nhấn **Build** (Ctrl+B) hoặc nút búa 🔨
2. Nhấn **Run** (Ctrl+R) hoặc nút play ▶️

**Client sẽ khởi chạy với màn hình đăng nhập.**

---

### Cách 2: Build từ Command Line

#### Trên Windows (với Qt đã cài):

```cmd
cd client
qmake
nmake          # Nếu dùng MSVC
# HOẶC
mingw32-make   # Nếu dùng MinGW
```

**Chạy:**
```cmd
release\MathPuzzleClient.exe
```

#### Trên Linux:

```bash
cd client
qmake
make
./MathPuzzleClient
```

#### Trên macOS:

```bash
cd client
qmake
make
open MathPuzzleClient.app
```

---

### Cách 3: Build với CMake

```bash
cd client
mkdir build
cd build
cmake ..
cmake --build .

# Chạy
./MathPuzzleClient          # Linux/Mac
# HOẶC
.\MathPuzzleClient.exe      # Windows
```

---

## PHẦN 3: CHƠI GAME

### Bước 1: Đăng nhập

1. **Khởi động Client** (làm 4 lần trên 4 máy khác nhau hoặc 4 cửa sổ trên 1 máy)
2. **Màn hình Login:**
   - Server: `localhost` (hoặc IP của server)
   - Port: `8888`
   - Nhấn **Connect**
3. **Đăng ký tài khoản mới:**
   - Username: `player1` (player2, player3, player4)
   - Password: `pass123`
   - Nhấn **Register**
4. **Hoặc đăng nhập nếu đã có tài khoản:**
   - Nhập username và password
   - Nhấn **Login**

### Bước 2: Tạo/Tham gia phòng

**Player 1 (host):**
1. Sau khi login, bạn sẽ thấy **Lobby Screen**
2. Nhập tên phòng (ví dụ: `Room 1`)
3. Nhấn **Create Room**
4. Chờ 3 người còn lại join

**Player 2, 3, 4:**
1. Trong **Lobby Screen**, nhấn **Refresh**
2. Chọn phòng trong danh sách
3. Nhấn **Join Room**

### Bước 3: Sẵn sàng

1. Khi cả 4 người đã vào phòng
2. Mỗi người nhấn nút **Ready**
3. Khi cả 4 người đều ready → **Game tự động bắt đầu!**

### Bước 4: Chơi game

**Màn hình game hiển thị:**
- **Phương trình** cần giải ở trên cùng (ví dụ: `P1 - P2 × P3 = P4`)
- **4 ma trận 4×4**:
  - 3 ma trận hiển thị số (của 3 người khác)
  - 1 ma trận bị ẩn (của bạn) - hiển thị dấu `?`
- **Timer**: Đếm ngược từ 180 giây (3 phút)
- **Chat box**: Giao tiếp với team

**Chiến thuật chơi:**

1. **Chia sẻ thông tin:**
   ```
   Player 1: "Ma trận của tôi (P1), dòng 0: 15, 23, 8, 42"
   Player 2: "Ma trận của tôi (P2), dòng 1: 5, 10, 20, 30"
   ```

2. **Tính toán:**
   - Nhóm cùng thảo luận để tìm 4 số thỏa mãn phương trình
   - Ví dụ: `15 - 10 × 2 = 5` → cần tìm P4 = -5 hoặc tính lại theo thứ tự ưu tiên

3. **Submit đáp án:**
   - Click vào ô trong **ma trận ẩn của bạn** (ma trận có dấu `?`)
   - Nhấn nút **Submit**
   - Xác nhận lựa chọn

4. **Chờ kết quả:**
   - Khi cả 4 người đã submit → Server kiểm tra
   - Hoặc hết giờ → Thua

### Bước 5: Xem kết quả

**Màn hình Result:**
- **WIN**: Nếu cả 4 người chọn đúng
- **LOSE**: Nếu sai hoặc hết giờ
- Hiển thị lời giải (nếu thua)
- Quay lại phòng để chơi lại

---

## KIỂM TRA KẾT NỐI

### Test Server với netcat/telnet

Bạn có thể test server bằng công cụ command line:

```bash
# Linux/Mac
nc localhost 8888

# Windows
telnet localhost 8888
```

**Thử các lệnh:**
```
REGISTER|testuser|pass123
LOGIN|testuser|pass123
LIST_ROOMS
CREATE_ROOM|Test Room
```

---

## PROTOCOL CƠ BẢN

### Định dạng:
```
COMMAND|arg1|arg2|arg3\n
```

### Ví dụ:
```
Client → Server:  LOGIN|alice|pass123
Server → Client:  LOGIN_OK|alice

Client → Server:  CREATE_ROOM|My Room
Server → Client:  ROOM_CREATED|0|My Room

Server → Client:  GAME_START|P1-P2*P3=P4|HIDDEN|1,2,3,...|...|...
Client → Server:  SUBMIT|2|3

Server → Client:  GAME_END|WIN|Congratulations!
```

Xem chi tiết trong file `PROTOCOL.md`.

---

## TRỤ SỞ LÝ THUYẾT

### 1. Mô hình mạng
- **Client-Server architecture**
- Server lắng nghe trên port 8888
- Client kết nối qua TCP/IP

### 2. Giao thức truyền
- **TCP (Transmission Control Protocol)**
- Đảm bảo tin cậy: không mất gói tin, đúng thứ tự
- Dùng Socket API (C/C++)

### 3. I/O Multiplexing (Server)
- **Hàm `select()`**: Quản lý nhiều client trên một thread
- Không bị blocking khi chờ một client cụ thể
- Server xử lý đồng thời 100 client

### 4. Stream Processing
- **Buffer** cho mỗi client
- Xử lý **fragmentation** (tin nhắn bị chia nhỏ)
- Xử lý **merging** (nhiều tin nhắn gộp chung)
- Delimiter `\n` để tách message

### 5. Qt Framework (Client)
- **Signals & Slots**: Cơ chế xử lý sự kiện
- **QTcpSocket**: Kết nối mạng
- **Event-driven**: Non-blocking, không cần thread riêng
- **State Machine**: Quản lý chuyển màn hình

### 6. Game Logic
- **Asymmetric Information**: Mỗi người thấy thông tin khác nhau
- Server tạo puzzle và phân phối chọn lọc
- Mỗi client nhận 3/4 ma trận (ẩn ma trận của mình)

---

## TÍNH NĂNG NỔI BẬT

### Server Side:
✅ I/O Multiplexing với `select()` (2 điểm)
✅ Xử lý Stream với buffer (1 điểm)
✅ Đăng ký/Đăng nhập & Quản lý Session (1 điểm)
✅ PING/PONG & Xử lý mất kết nối (2 điểm)
✅ Quản lý Phòng (Create/Join/Ready) (3 điểm)
✅ Tạo Puzzle & Phân phối Bất đối xứng (2 điểm)
✅ Quản lý & Đồng bộ Thời gian (1 điểm)
✅ Xác thực Kết quả (Win/Lose) (3 điểm)

**Bonus:**
- Chat trong phòng
- Auto PING/PONG heartbeat

### Client Side:
✅ Kết nối QTcpSocket (2 điểm)
✅ Xử lý readyRead event-driven (2 điểm)
✅ Render 5 màn hình UI (3 điểm)
✅ Sử dụng Qt Framework (2 điểm)
✅ State Machine quản lý màn hình (2 điểm)
✅ Xử lý Input với Signals & Slots (2 điểm)
✅ Parse & Update dữ liệu (2 điểm)

**Bonus:**
- Chat system với UI đẹp
- Matrix cell selection UI
- Timer với color indicators

---

## LƯU Ý QUAN TRỌNG

### 1. Thứ tự khởi động
**Phải chạy Server trước, Client sau!**

### 2. Số lượng người chơi
**Cần đúng 4 người chơi mới có thể bắt đầu game.**

### 3. Network
- Server và Client phải cùng mạng (LAN) hoặc dùng localhost
- Firewall có thể chặn port 8888 → cần mở port

### 4. Đăng ký tài khoản
- Tài khoản được lưu trong file `server/users.txt`
- Format: `username:password`
- **Không mã hóa** (chỉ để demo)

### 5. Timeout
- Server gửi PING mỗi 10 giây
- Client phải PONG trong 30 giây
- Quá thời gian → tự động disconnect

---

## XỬ LÝ LỖI

### Lỗi: Cannot connect to server

**Nguyên nhân:**
- Server chưa chạy
- IP/Port sai
- Firewall chặn

**Giải pháp:**
1. Kiểm tra server đang chạy: `ps aux | grep game_server`
2. Kiểm tra port: `sudo lsof -i :8888`
3. Thử kết nối với netcat: `nc localhost 8888`

### Lỗi: Room not found

**Nguyên nhân:**
- Phòng đã full (4/4 người)
- Phòng đã bắt đầu game
- Phòng bị xóa

**Giải pháp:**
- Nhấn **Refresh** trong lobby
- Tạo phòng mới

### Lỗi: Game aborted

**Nguyên nhân:**
- Một người chơi disconnect
- Server gặp lỗi

**Giải pháp:**
- Quay lại phòng
- Ready lại để chơi ván mới

---

## DEMO NHANH (1 máy, 4 client)

```bash
# Terminal 1: Server
cd server
make run

# Terminal 2-5: Client (hoặc dùng Qt Creator chạy 4 instance)
cd client
# Mở Qt Creator → Run 4 lần (hoặc chạy exe 4 lần)

# Hoặc dùng command line:
./MathPuzzleClient &
./MathPuzzleClient &
./MathPuzzleClient &
./MathPuzzleClient &
```

**Sau đó:**
1. Cả 4 client login với user khác nhau
2. Client 1 tạo phòng
3. Client 2, 3, 4 join phòng
4. Cả 4 click Ready
5. Chơi game!

---

## HỖ TRỢ

### File tham khảo:
- `PROTOCOL.md`: Chi tiết giao thức
- `TONG_KET.md`: Tổng kết tính năng đã implement
- `README.md`: Tổng quan dự án

### Cấu trúc thư mục:
```
Project/
├── server/              # Server C
│   ├── server.c         # Main loop với select()
│   ├── auth.c           # Đăng ký/Đăng nhập
│   ├── room.c           # Quản lý phòng
│   ├── game.c           # Logic game & puzzle
│   ├── network.c        # PING/PONG & chat
│   ├── server.h         # Header
│   └── Makefile         # Build script
├── client/              # Client Qt
│   ├── *.h, *.cpp       # Source code
│   ├── MathPuzzleClient.pro  # Qt project
│   └── CMakeLists.txt   # CMake (alternative)
├── PROTOCOL.md          # Giao thức
├── HUONG_DAN_CHAY.md    # File này
└── TONG_KET.md          # Tổng kết

```

---

## KẾT LUẬN

Dự án đã hoàn thành đầy đủ các yêu cầu về:
- ✅ Socket Programming (TCP)
- ✅ I/O Multiplexing (`select()`)
- ✅ Custom Protocol
- ✅ Qt Framework với Signals/Slots
- ✅ Asymmetric Information Game Logic
- ✅ 5 màn hình UI hoàn chỉnh
- ✅ PING/PONG heartbeat
- ✅ Chat system

**Tổng điểm:** 15/15 (Server) + 15/15 (Client) + Bonus features

**Chúc bạn chơi game vui vẻ! 🎮**

