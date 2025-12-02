# CẤU TRÚC VÀ CƠ CHẾ HOẠT ĐỘNG CỦA SERVER

## MỤC LỤC
1. [Tổng quan](#1-tổng-quan)
2. [Kiến trúc hệ thống](#2-kiến-trúc-hệ-thống)
3. [Cấu trúc dữ liệu](#3-cấu-trúc-dữ-liệu)
4. [Các module chính](#4-các-module-chính)
5. [Flow hoạt động](#5-flow-hoạt-động)
6. [Protocol giao tiếp](#6-protocol-giao-tiếp)
7. [Cơ chế xử lý đa client](#7-cơ-chế-xử-lý-đa-client)
8. [Cơ chế game](#8-cơ-chế-game)

---

## 1. TỔNG QUAN

### 1.1. Mô tả
**Math Puzzle Game Server** là server game multiplayer được viết bằng **C thuần**, sử dụng **BSD Socket API** để xử lý kết nối mạng. Server hỗ trợ nhiều phòng chơi đồng thời, mỗi phòng có tối đa 4 người chơi.

### 1.2. Thông số kỹ thuật
```c
#define PORT 8888                // Cổng mặc định
#define MAX_CLIENTS 100          // Tối đa 100 clients đồng thời
#define MAX_ROOMS 25             // Tối đa 25 phòng
#define PLAYERS_PER_ROOM 4       // 4 người chơi/phòng
#define GAME_DURATION 180        // 3 phút/ván chơi
#define PING_INTERVAL 10         // Ping mỗi 10 giây
#define PING_TIMEOUT 30          // Timeout sau 30 giây
#define RECONNECT_TIMEOUT 60     // Cho phép reconnect trong 60 giây
```

### 1.3. Công nghệ sử dụng
- **Ngôn ngữ**: C (C99 standard)
- **Socket API**: POSIX/BSD Socket
- **I/O Model**: I/O Multiplexing với `select()`
- **Concurrency**: Single-process, event-driven
- **File I/O**: Text-based user database

---

## 2. KIẾN TRÚC HỆ THỐNG

### 2.1. Mô hình kiến trúc

```
┌──────────────────────────────────────────────────────────┐
│                    SERVER PROCESS                        │
│                  (Single Process/Thread)                 │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  ┌────────────┐    ┌─────────────────────────────┐     │
│  │ Listen     │───▶│   select() Event Loop       │     │
│  │ Socket     │    │   (I/O Multiplexing)        │     │
│  └────────────┘    └─────────────────────────────┘     │
│                            │                            │
│         ┌──────────────────┼──────────────────┐        │
│         ▼                  ▼                  ▼        │
│  ┌─────────────┐   ┌─────────────┐   ┌─────────────┐  │
│  │  Client 1   │   │  Client 2   │   │  Client N   │  │
│  │  Socket FD  │   │  Socket FD  │   │  Socket FD  │  │
│  └─────────────┘   └─────────────┘   └─────────────┘  │
│         │                  │                  │        │
│         └──────────────────┼──────────────────┘        │
│                            ▼                            │
│              ┌─────────────────────────┐               │
│              │   Message Handler       │               │
│              └─────────────────────────┘               │
│                            │                            │
│         ┌──────────────────┼──────────────────┐        │
│         ▼                  ▼                  ▼        │
│  ┌───────────┐      ┌──────────┐      ┌───────────┐   │
│  │   Auth    │      │   Room   │      │   Game    │   │
│  │  Module   │      │  Module  │      │  Module   │   │
│  └───────────┘      └──────────┘      └───────────┘   │
└──────────────────────────────────────────────────────────┘
```

### 2.2. Đặc điểm kiến trúc

#### **Single-Process Architecture**
- ✅ Chỉ 1 process xử lý tất cả
- ✅ Không dùng fork() hay thread
- ✅ Event-driven với select()

#### **Ưu điểm**
- Đơn giản, dễ debug
- Không cần lo locking/synchronization
- Tiết kiệm RAM
- Phù hợp với quy mô game nhỏ

#### **Nhược điểm**
- Giới hạn ~1024 file descriptors (FD_SETSIZE)
- CPU-bound task sẽ block toàn bộ server
- Không tận dụng multi-core

---

## 3. CẤU TRÚC DỮ LIỆU

### 3.1. Client Structure

```c
typedef struct {
    int socket_fd;                  // File descriptor của socket
    int active;                     // 0 = inactive, 1 = active
    char username[MAX_USERNAME];    // Tên đăng nhập
    char recv_buffer[BUFFER_SIZE];  // Buffer nhận dữ liệu
    int buffer_len;                 // Độ dài data trong buffer
    ClientState state;              // Trạng thái hiện tại
    int room_id;                    // ID phòng đang ở (-1 = không trong phòng)
    int player_index;               // Vị trí trong phòng (0-3)
    time_t last_pong_time;          // Thời điểm PONG cuối
    time_t last_ping_time;          // Thời điểm PING cuối
    time_t disconnect_time;         // Thời điểm disconnect
    ClientState saved_state;        // State trước khi disconnect
} Client;
```

**Client States:**
```c
typedef enum {
    STATE_CONNECTED,      // Vừa kết nối, chưa auth
    STATE_AUTHENTICATED,  // Đã auth, chưa vào lobby
    STATE_IN_LOBBY,       // Đang ở lobby
    STATE_IN_ROOM,        // Đang ở phòng, chưa ready
    STATE_READY,          // Đã ready, chờ game bắt đầu
    STATE_IN_GAME,        // Đang chơi game
    STATE_DISCONNECTED    // Tạm thời disconnect (cho phép reconnect)
} ClientState;
```

### 3.2. Room Structure

```c
typedef struct {
    int id;                                     // Room ID
    char name[MAX_ROOM_NAME];                   // Tên phòng
    int active;                                 // 0 = inactive, 1 = active
    int player_ids[PLAYERS_PER_ROOM];          // Mảng client_idx của 4 players
    int player_ready[PLAYERS_PER_ROOM];        // Ready status
    int player_count;                           // Số lượng player hiện tại
    int game_started;                           // Game đã bắt đầu chưa
    int host_index;                             // Index của host (0-3)
    Puzzle puzzle;                              // Puzzle hiện tại
    time_t game_start_time;                     // Thời điểm bắt đầu game
    int game_time_remaining;                    // Thời gian còn lại (giây)
    int submitted_answers[PLAYERS_PER_ROOM][2]; // Câu trả lời [row, col]
    int answer_submitted[PLAYERS_PER_ROOM];     // Đã submit chưa
    int all_submitted;                          // Tất cả đã submit chưa
    int current_round;                          // Round hiện tại (1-5)
    int total_rounds;                           // Tổng số round (mặc định 5)
} Room;
```

### 3.3. Puzzle Structure

```c
typedef struct {
    Operator op1;                              // Toán tử 1
    Operator op2;                              // Toán tử 2
    Operator op3;                              // Toán tử 3 (cho format 3 operators)
    EquationFormat format;                     // Định dạng phương trình
    Matrix matrices[PLAYERS_PER_ROOM];        // 4 ma trận 4x4
    int solution_row[PLAYERS_PER_ROOM];       // Hàng chứa đáp án
    int solution_col[PLAYERS_PER_ROOM];       // Cột chứa đáp án
    int solution_values[PLAYERS_PER_ROOM];    // Giá trị đáp án (P1, P2, P3, P4)
    int result;                                // Kết quả phương trình
    int round;                                 // Round hiện tại (1-5)
} Puzzle;
```

**Equation Formats:**
```c
typedef enum {
    FORMAT_P1_P2_P3_EQ_P4,   // P1 ± P2 ± P3 = P4
    FORMAT_P1_EQ_P2_P3_P4,   // P1 = P2 ± P3 ± P4
    FORMAT_P1_P2_EQ_P3_P4    // P1 ± P2 = P3 ± P4
} EquationFormat;
```

### 3.4. Server Structure

```c
typedef struct {
    int listen_fd;               // Listening socket FD
    Client clients[MAX_CLIENTS]; // Mảng tất cả clients
    Room rooms[MAX_ROOMS];       // Mảng tất cả rooms
    fd_set master_set;           // Master FD set cho select()
    int max_fd;                  // FD lớn nhất hiện tại
    time_t last_tick_time;       // Thời điểm tick cuối cùng
} Server;
```

---

## 4. CÁC MODULE CHÍNH

Server được chia thành 5 module chính:

### 4.1. server.c - Core Module
**Chức năng:**
- Khởi tạo server
- Main event loop với select()
- Accept client mới
- Xử lý disconnect/reconnect
- Parse và route messages

**Các hàm chính:**
```c
void server_init(Server *server);           // Khởi tạo server
void server_run(Server *server);            // Main loop
int client_accept(Server *server);          // Accept connection mới
void client_disconnect(Server *, int idx);  // Disconnect vĩnh viễn
void client_mark_disconnected(Server *, int); // Disconnect tạm thời
void client_process_data(Server *, int);    // Xử lý data từ client
void handle_message(Server *, int, char*);  // Parse và route message
```

### 4.2. auth.c - Authentication Module
**Chức năng:**
- Đăng ký user mới
- Xác thực đăng nhập
- Xử lý reconnection
- Quản lý file users.txt

**Các hàm chính:**
```c
int register_user(const char *username, const char *password);
int authenticate_user(const char *username, const char *password);
void handle_register(Server *, int client_idx, const char *user, const char *pass);
void handle_login(Server *, int client_idx, const char *user, const char *pass);
```

**Cơ chế reconnection:**
- Client disconnect → state = STATE_DISCONNECTED
- Giữ session trong 60 giây
- Login lại → restore state và room_id
- Notify players khác về reconnection

### 4.3. room.c - Room Management Module
**Chức năng:**
- Tạo/xóa room
- Join/leave room
- Quản lý player ready
- Start game (host only)
- Broadcast messages trong room

**Các hàm chính:**
```c
int room_create(Server *, const char *name);          // Tạo phòng
int room_join(Server *, int room_id, int client_idx); // Join phòng
void handle_leave_room(Server *, int client_idx);     // Leave phòng
void handle_ready(Server *, int client_idx);          // Toggle ready
void handle_start_game(Server *, int client_idx);     // Bắt đầu game (host)
void room_broadcast(Server *, int room_id, const char *msg, int exclude);
void send_room_status(Server *, int room_id);         // Gửi trạng thái phòng
```

**Cơ chế host:**
- Player đầu tiên join = host
- Host có quyền start game
- Nếu host leave → chọn host mới tự động

### 4.4. game.c - Game Logic Module
**Chức năng:**
- Generate puzzle theo độ khó
- Gửi puzzle đến clients (asymmetric info)
- Verify câu trả lời
- Quản lý round system (5 rounds)
- End game và cleanup

**Các hàm chính:**
```c
void puzzle_generate(Puzzle *puzzle, int round);              // Gen puzzle
void puzzle_send_to_clients(Server *, int room_id);          // Gửi puzzle
void room_start_game(Server *, int room_id);                 // Bắt đầu game
void room_end_game(Server *, int room_id, int won, int timeout);
int puzzle_verify_solution(Puzzle *, int submitted[4][2]);   // Verify đáp án
void handle_submit(Server *, int client_idx, int row, int col);
```

**Round System:**
```
Round 1: Easy       - Cộng/trừ, số nhỏ (1-50)
Round 2: Medium     - Cộng/trừ, số lớn (10-80)
Round 3: Hard       - Có nhân, số vừa (2-30)
Round 4: Very Hard  - Mix operators (2-40)
Round 5: Expert     - All operators + số âm (-20 to 50)
```

### 4.5. network.c - Network Utilities Module
**Chức năng:**
- PING/PONG keep-alive
- Chat messages
- Timeout detection
- Server shutdown

**Các hàm chính:**
```c
void send_ping_to_all(Server *);           // Gửi PING định kỳ
void check_ping_timeouts(Server *);        // Kiểm tra timeout
void handle_pong(Server *, int client_idx); // Xử lý PONG response
void handle_chat(Server *, int client_idx, const char *message);
void server_shutdown(Server *);            // Shutdown gracefully
```

---

## 5. FLOW HOẠT ĐỘNG

### 5.1. Server Initialization Flow

```
main()
  │
  ├─▶ srand(time(NULL))              // Seed random
  │
  ├─▶ server_init(&server)
  │     │
  │     ├─▶ socket()                 // Tạo listening socket
  │     ├─▶ setsockopt(SO_REUSEADDR) // Cho phép reuse port
  │     ├─▶ bind()                   // Bind port 8888
  │     ├─▶ listen()                 // Lắng nghe
  │     ├─▶ FD_ZERO(&master_set)     // Init fd_set
  │     ├─▶ FD_SET(listen_fd)        // Add listen_fd
  │     └─▶ Initialize clients[] & rooms[]
  │
  └─▶ server_run(&server)            // Main loop
```

### 5.2. Main Event Loop Flow

```
while (1) {
    ┌──────────────────────────────────────┐
    │  1. select() với timeout 1 giây     │
    │     - Chờ sự kiện I/O                │
    │     - Timeout để chạy periodic tasks │
    └──────────────────────────────────────┘
                    │
        ┌───────────┴───────────┐
        ▼                       ▼
    ┌─────────────┐      ┌────────────────────┐
    │ New Client? │      │ Existing Clients   │
    │ FD_ISSET    │      │ FD_ISSET per client│
    │ (listen_fd) │      └────────────────────┘
    └─────────────┘                 │
         │                          │
         ▼                          ▼
    client_accept()        client_process_data()
                                    │
                                    ▼
                            handle_message()
                                    │
                    ┌───────────────┼───────────────┐
                    ▼               ▼               ▼
              handle_login()  handle_join()  handle_submit()
              
    ┌──────────────────────────────────────┐
    │  2. Periodic Tasks (mỗi 1 giây)     │
    │     - Update game timers             │
    │     - Send room status (2s)          │
    │     - Check ping timeouts            │
    │     - Check reconnect timeouts       │
    │     - Send PING (10s interval)       │
    └──────────────────────────────────────┘
}
```

### 5.3. Client Connection Flow

```
Client Connect
    │
    ▼
┌─────────────────────────────────┐
│ 1. accept() new connection      │
│    - Find free slot in clients[]│
│    - Init Client structure      │
│    - FD_SET(client_fd)          │
│    - state = STATE_CONNECTED    │
└─────────────────────────────────┘
    │
    ▼
Send "WELCOME|Math Puzzle..."
    │
    ▼
┌─────────────────────────────────┐
│ 2. Client sends LOGIN/REGISTER  │
└─────────────────────────────────┘
    │
    ├─▶ REGISTER
    │   └─▶ register_user()
    │       ├─▶ Check users.txt
    │       ├─▶ Append new user
    │       └─▶ Send "REGISTER_OK"
    │
    └─▶ LOGIN
        └─▶ authenticate_user()
            ├─▶ Check users.txt
            ├─▶ Check reconnection
            │   └─▶ Restore session nếu có
            ├─▶ state = STATE_IN_LOBBY
            └─▶ Send "LOGIN_OK|username"
                Send room list
```

### 5.4. Game Flow

```
Player in Lobby
    │
    ▼
CREATE_ROOM or JOIN_ROOM
    │
    ▼
┌────────────────────────────┐
│ Player in Room             │
│ - Host or regular player   │
│ - state = STATE_IN_ROOM    │
└────────────────────────────┘
    │
    ▼
Toggle READY
    │
    ├─▶ Non-host: Wait cho host start
    │
    └─▶ Host: Send START_GAME
            │
            ▼
    ┌───────────────────────────────┐
    │ room_start_game()             │
    │ - current_round = 1           │
    │ - puzzle_generate(round)      │
    │ - puzzle_send_to_clients()    │
    │   (mỗi player thấy 3/4 matrix)│
    │ - state = STATE_IN_GAME       │
    │ - Start timer (180s)          │
    └───────────────────────────────┘
            │
            ▼
    Players discuss và submit
            │
            ▼
    ┌───────────────────────────────┐
    │ All players submitted         │
    │ OR timeout                    │
    └───────────────────────────────┘
            │
            ▼
    puzzle_verify_solution()
            │
        ┌───┴────┐
        ▼        ▼
     CORRECT  WRONG/TIMEOUT
        │        │
        │        └─▶ Send solution
        │            GAME_END|LOSE
        │            Reset room
        │
        ▼
    current_round < 5?
        │
        ├─▶ YES: Continue next round
        │         (loop back to puzzle_generate)
        │
        └─▶ NO:  All rounds complete!
                 GAME_END|WIN
                 Reset room
```

### 5.5. Message Processing Flow

```
recv() data from client
    │
    ▼
Append to recv_buffer[client_idx]
    │
    ▼
┌────────────────────────────────┐
│ Parse buffer line by line (\n)│
└────────────────────────────────┘
    │
    ▼
handle_message(server, client_idx, line)
    │
    ▼
sscanf(line, "%[^|]|%[^|]|%s", cmd, arg1, arg2)
    │
    ├─▶ "REGISTER" → handle_register()
    ├─▶ "LOGIN"    → handle_login()
    ├─▶ "CREATE_ROOM" → handle_create_room()
    ├─▶ "JOIN_ROOM"   → handle_join_room()
    ├─▶ "LEAVE_ROOM"  → handle_leave_room()
    ├─▶ "LIST_ROOMS"  → send_room_list()
    ├─▶ "READY"       → handle_ready()
    ├─▶ "START_GAME"  → handle_start_game()
    ├─▶ "SUBMIT"      → handle_submit()
    ├─▶ "PONG"        → handle_pong()
    ├─▶ "CHAT"        → handle_chat()
    └─▶ Unknown       → "ERROR|Unknown command"
```

---

## 6. PROTOCOL GIAO TIẾP

Server sử dụng **text-based protocol** với delimiter `|` và newline `\n`.

### 6.1. Client → Server

| Command | Format | Mô tả |
|---------|--------|-------|
| REGISTER | `REGISTER\|username\|password` | Đăng ký tài khoản |
| LOGIN | `LOGIN\|username\|password` | Đăng nhập |
| LIST_ROOMS | `LIST_ROOMS` | Lấy danh sách phòng |
| CREATE_ROOM | `CREATE_ROOM\|room_name` | Tạo phòng mới |
| JOIN_ROOM | `JOIN_ROOM\|room_id` | Vào phòng |
| LEAVE_ROOM | `LEAVE_ROOM` | Rời phòng |
| READY | `READY` | Toggle ready state |
| START_GAME | `START_GAME` | Bắt đầu game (host only) |
| SUBMIT | `SUBMIT\|row\|col` | Submit đáp án |
| CHAT | `CHAT\|message` | Gửi chat |
| PONG | `PONG` | Response to PING |

### 6.2. Server → Client

| Message | Format | Mô tả |
|---------|--------|-------|
| WELCOME | `WELCOME\|message` | Sau khi connect |
| LOGIN_OK | `LOGIN_OK\|username` | Login thành công |
| REGISTER_OK | `REGISTER_OK\|message` | Đăng ký thành công |
| RECONNECT_OK | `RECONNECT_OK\|username` | Reconnect thành công |
| ERROR | `ERROR\|message` | Thông báo lỗi |
| ROOM_LIST | `ROOM_LIST\|id:name:count\|...` | Danh sách phòng |
| ROOM_CREATED | `ROOM_CREATED\|room_id\|name` | Phòng đã tạo |
| ROOM_JOINED | `ROOM_JOINED\|room_id` | Join thành công |
| LEFT_ROOM | `LEFT_ROOM` | Leave thành công |
| PLAYER_JOINED | `PLAYER_JOINED\|index\|username` | Player mới join |
| PLAYER_LEFT | `PLAYER_LEFT\|username` | Player leave |
| PLAYER_DISCONNECTED | `PLAYER_DISCONNECTED\|username` | Player mất kết nối |
| PLAYER_RECONNECTED | `PLAYER_RECONNECTED\|username` | Player reconnect |
| ROOM_STATUS | `ROOM_STATUS\|count\|host_idx\|idx:name:ready:ping\|...` | Trạng thái phòng |
| GAME_START | `GAME_START\|equation\|matrix0\|matrix1\|matrix2\|matrix3\|round\|total` | Bắt đầu game |
| TIMER | `TIMER\|seconds` | Cập nhật timer |
| PLAYER_SUBMITTED | `PLAYER_SUBMITTED\|index\|username` | Player đã submit |
| CHAT | `CHAT\|username\|message` | Chat message |
| GAME_END | `GAME_END\|WIN\|message` hoặc `GAME_END\|LOSE\|reason\|solution` | Kết thúc game |
| GAME_ABORTED | `GAME_ABORTED\|reason` | Game bị hủy |
| PING | `PING` | Keep-alive ping |

### 6.3. Ví dụ Protocol Flow

**Scenario: Player login và join room**

```
Client → Server: LOGIN|alice|pass123
Server → Client: LOGIN_OK|alice
Server → Client: ROOM_LIST|0:Room1:2|1:Room2:1

Client → Server: JOIN_ROOM|0
Server → Client: ROOM_JOINED|0
Server → All in room: PLAYER_JOINED|2|alice
Server → All in room: ROOM_STATUS|3|0|0:bob:1:45|1:charlie:0:62|2:alice:0:12

Client → Server: READY
Server → All in room: ROOM_STATUS|3|0|0:bob:1:45|1:charlie:0:62|2:alice:1:15
```

**Scenario: Start game và submit**

```
Host → Server: START_GAME
Server → Player 0: GAME_START|P1+P2-P3=P4|HIDDEN|1,2,3...|4,5,6...|7,8,9...|1|5
Server → Player 1: GAME_START|P1+P2-P3=P4|1,2,3...|HIDDEN|4,5,6...|7,8,9...|1|5
Server → Player 2: GAME_START|P1+P2-P3=P4|1,2,3...|4,5,6...|HIDDEN|7,8,9...|1|5
Server → Player 3: GAME_START|P1+P2-P3=P4|1,2,3...|4,5,6...|7,8,9...|HIDDEN|1|5

(Every second)
Server → All: TIMER|179
Server → All: TIMER|178
...

Player 0 → Server: SUBMIT|2|3
Server → All: PLAYER_SUBMITTED|0|bob

(When all submitted)
Server → All: GAME_END|WIN|Round 1 complete! Starting round 2...
(hoặc)
Server → All: GAME_END|LOSE|Wrong answer!|P1[2,3]=15 + P2[1,1]=8 - P3[0,2]=3 = P4[3,1]=20
```

---

## 7. CƠ CHẾ XỬ LÝ ĐA CLIENT

### 7.1. I/O Multiplexing với select()

Server sử dụng **select()** để giám sát nhiều socket đồng thời trong 1 process.

```c
void server_run(Server *server) {
    while (1) {
        // Copy master set (vì select() modify nó)
        fd_set read_fds = server->master_set;
        
        // Timeout 1 giây để chạy periodic tasks
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        // Block cho đến khi có sự kiện I/O hoặc timeout
        int activity = select(server->max_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            perror("select");
            continue;
        }
        
        // Kiểm tra new connection
        if (FD_ISSET(server->listen_fd, &read_fds)) {
            client_accept(server);
        }
        
        // Kiểm tra data từ existing clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (server->clients[i].active && 
                FD_ISSET(server->clients[i].socket_fd, &read_fds)) {
                client_process_data(server, i);
            }
        }
        
        // Periodic tasks mỗi giây
        // ...
    }
}
```

**Cách hoạt động:**

1. **FD_SET Management:**
   - `master_set`: Giữ tất cả FDs cần monitor
   - `read_fds`: Copy của master_set (vì select() modify)
   - Mỗi lần accept: `FD_SET(new_fd, &master_set)`
   - Mỗi lần close: `FD_CLR(fd, &master_set)`

2. **select() Call:**
   ```c
   int select(int max_fd + 1,      // Số FDs cần check
              fd_set *readfds,     // FDs cần check cho read
              fd_set *writefds,    // FDs cần check cho write (NULL)
              fd_set *exceptfds,   // FDs cần check cho exception (NULL)
              struct timeval *tv); // Timeout
   ```

3. **Return Value:**
   - `> 0`: Số FDs ready
   - `0`: Timeout
   - `-1`: Error

4. **FD_ISSET Check:**
   - Sau select(), loop qua tất cả FDs
   - `FD_ISSET(fd, &read_fds)` → FD này có data

### 7.2. Buffer Management

Mỗi client có buffer riêng để xử lý **partial messages**:

```c
void client_process_data(Server *server, int client_idx) {
    Client *client = &server->clients[client_idx];
    
    // 1. Đọc data từ socket
    char temp_buf[BUFFER_SIZE];
    int bytes_read = recv(client->socket_fd, temp_buf, sizeof(temp_buf) - 1, 0);
    
    if (bytes_read <= 0) {
        // Connection closed
        client_mark_disconnected(server, client_idx);
        return;
    }
    
    // 2. Append vào buffer của client
    memcpy(client->recv_buffer + client->buffer_len, temp_buf, bytes_read);
    client->buffer_len += bytes_read;
    client->recv_buffer[client->buffer_len] = '\0';
    
    // 3. Parse complete messages (delimiter = \n)
    char *line_start = client->recv_buffer;
    char *line_end;
    
    while ((line_end = strchr(line_start, '\n')) != NULL) {
        *line_end = '\0';  // Null-terminate
        
        // Process message
        handle_message(server, client_idx, line_start);
        
        line_start = line_end + 1;
    }
    
    // 4. Move remaining data lên đầu buffer
    int remaining = strlen(line_start);
    if (remaining > 0) {
        memmove(client->recv_buffer, line_start, remaining);
    }
    client->buffer_len = remaining;
}
```

**Tại sao cần buffer?**
- TCP stream không đảm bảo message boundaries
- 1 recv() có thể nhận:
  - Partial message: `"LOGIN|ali"`
  - Multiple messages: `"LOGIN|alice|pass\nREADY\n"`
  - Mixed: `"...alice\nLIST_ROO"`

### 7.3. Disconnect & Reconnect

#### **Temporary Disconnect:**

```c
void client_mark_disconnected(Server *server, int client_idx) {
    Client *client = &server->clients[client_idx];
    
    // Save state trước khi disconnect
    client->saved_state = client->state;
    client->state = STATE_DISCONNECTED;
    client->disconnect_time = time(NULL);
    
    // Close socket nhưng GIỮ client data
    FD_CLR(client->socket_fd, &server->master_set);
    close(client->socket_fd);
    client->socket_fd = -1;
    
    // Notify room nếu có
    if (client->room_id >= 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "PLAYER_DISCONNECTED|%s\n", client->username);
        room_broadcast(server, client->room_id, msg, client_idx);
    }
}
```

#### **Reconnection:**

```c
void handle_login(...) {
    // Check disconnected clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].state == STATE_DISCONNECTED &&
            strcmp(server->clients[i].username, username) == 0) {
            
            time_t elapsed = time(NULL) - server->clients[i].disconnect_time;
            if (elapsed < RECONNECT_TIMEOUT) {
                // RESTORE SESSION
                client->username = old_username;
                client->state = old_saved_state;
                client->room_id = old_room_id;
                client->player_index = old_player_index;
                
                // Update room
                room->player_ids[old_player_index] = new_client_idx;
                
                // Notify
                room_broadcast(server, room_id, "PLAYER_RECONNECTED|...");
            }
        }
    }
}
```

#### **Timeout Check:**

```c
void check_reconnect_timeouts(Server *server) {
    time_t now = time(NULL);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].state == STATE_DISCONNECTED) {
            if (now - clients[i].disconnect_time >= RECONNECT_TIMEOUT) {
                // Permanently disconnect
                client_disconnect(server, i);
            }
        }
    }
}
```

### 7.4. Keep-Alive Mechanism

```c
// Mỗi 10 giây
void send_ping_to_all(Server *server) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].active) {
            client_send(&server->clients[i], "PING\n");
            clients[i].last_ping_time = time(NULL);
        }
    }
}

// Client response
void handle_pong(Server *server, int client_idx) {
    clients[client_idx].last_pong_time = time(NULL);
}

// Check timeout mỗi giây
void check_ping_timeouts(Server *server) {
    time_t now = time(NULL);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            if (now - clients[i].last_pong_time > PING_TIMEOUT) {
                printf("Client timeout\n");
                client_disconnect(server, i);
            }
        }
    }
}
```

---

## 8. CƠ CHẾ GAME

### 8.1. Puzzle Generation

Game có **5 rounds**, mỗi round khó hơn:

```c
void puzzle_generate(Puzzle *puzzle, int round) {
    switch (round) {
        case 1: // Easy
            format = FORMAT_P1_P2_P3_EQ_P4;  // P1 + P2 - P3 = P4
            op1 = ADD hoặc SUB;
            op2 = ADD hoặc SUB;
            range = 1-50;
            break;
            
        case 2: // Medium
            format = FORMAT_P1_P2_EQ_P3_P4;  // P1 + P2 = P3 - P4
            op1, op2 = ADD hoặc SUB;
            range = 10-80;
            break;
            
        case 3: // Hard
            format = FORMAT_P1_EQ_P2_P3_P4;  // P1 = P2 * P3 + P4
            op1 = MUL;
            op2 = ADD hoặc SUB;
            range = 2-30;
            break;
            
        case 4: // Very Hard
            format = FORMAT_P1_P2_EQ_P3_P4;  // P1 * P2 = P3 * P4
            op1 = MUL;
            op2 = ADD, SUB, hoặc MUL;
            range = 2-40;
            break;
            
        case 5: // Expert
            format = FORMAT_P1_P2_P3_EQ_P4;  // P1 * P2 / P3 = P4
            op1 = MUL hoặc DIV;
            op2 = ADD, SUB, hoặc MUL;
            range = -20 to 50 (có số âm);
            allow_negative = 1;
            break;
    }
    
    // Generate P1, P2, P3, P4 dựa theo format
    // ...
    
    // Generate 4 ma trận 4x4 với random numbers
    for (int m = 0; m < 4; m++) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                matrix[m][i][j] = random(range);
            }
        }
        
        // Đặt solution value vào vị trí ngẫu nhiên
        solution_row[m] = rand() % 4;
        solution_col[m] = rand() % 4;
        matrix[m][solution_row[m]][solution_col[m]] = solution_values[m];
    }
}
```

### 8.2. Asymmetric Information

**Core mechanic:** Mỗi player thấy 3/4 ma trận (ẩn ma trận của mình).

```c
void puzzle_send_to_clients(Server *server, int room_id) {
    Puzzle *puzzle = &room->puzzle;
    
    for (int player = 0; player < 4; player++) {
        char buffer[BUFFER_SIZE];
        
        // Build message: GAME_START|equation|matrix0|matrix1|matrix2|matrix3
        sprintf(buffer, "GAME_START|%s", equation);
        
        for (int m = 0; m < 4; m++) {
            if (m == player) {
                // Ẩn ma trận của player này
                strcat(buffer, "|HIDDEN");
            } else {
                // Gửi đầy đủ ma trận
                strcat(buffer, "|");
                for (int i = 0; i < 4; i++) {
                    for (int j = 0; j < 4; j++) {
                        sprintf(temp, "%d,", matrix[m][i][j]);
                        strcat(buffer, temp);
                    }
                }
            }
        }
        
        strcat(buffer, "|1|5\n");  // round|total_rounds
        client_send(&clients[room->player_ids[player]], buffer);
    }
}
```

**Example:**
- Equation: `P1 + P2 - P3 = P4`
- Player 0 thấy: Matrix1, Matrix2, Matrix3 (ẩn Matrix0)
- Player 1 thấy: Matrix0, Matrix2, Matrix3 (ẩn Matrix1)
- Player 2 thấy: Matrix0, Matrix1, Matrix3 (ẩn Matrix2)
- Player 3 thấy: Matrix0, Matrix1, Matrix2 (ẩn Matrix3)

→ **Bắt buộc phải communicate!**

### 8.3. Answer Verification

```c
int puzzle_verify_solution(Puzzle *puzzle, int submitted[4][2]) {
    // Check tất cả 4 players
    for (int i = 0; i < 4; i++) {
        int row = submitted[i][0];
        int col = submitted[i][1];
        
        // So sánh với đáp án đúng
        if (row != puzzle->solution_row[i] || 
            col != puzzle->solution_col[i]) {
            return 0;  // WRONG
        }
    }
    return 1;  // CORRECT
}

void handle_submit(Server *server, int client_idx, int row, int col) {
    // Lưu đáp án
    room->submitted_answers[player_idx][0] = row;
    room->submitted_answers[player_idx][1] = col;
    room->answer_submitted[player_idx] = 1;
    
    // Notify tất cả
    room_broadcast(server, room_id, "PLAYER_SUBMITTED|...");
    
    // Check nếu tất cả đã submit
    int all_submitted = 1;
    for (int i = 0; i < 4; i++) {
        if (room->player_ids[i] >= 0 && !room->answer_submitted[i]) {
            all_submitted = 0;
            break;
        }
    }
    
    if (all_submitted) {
        // Verify
        int correct = puzzle_verify_solution(&room->puzzle, 
                                            room->submitted_answers);
        room_end_game(server, room_id, correct, 0);
    }
}
```

### 8.4. Round System

```c
void room_end_game(Server *server, int room_id, int won, int timeout) {
    Room *room = &server->rooms[room_id];
    
    if (won) {
        if (room->current_round < room->total_rounds) {
            // Continue to next round
            room_broadcast(server, room_id, 
                "GAME_END|WIN|Round complete! Starting next...\n");
            
            room->current_round++;
            room_start_game(server, room_id);  // Recursive
            return;
        } else {
            // All rounds complete
            room_broadcast(server, room_id,
                "GAME_END|WIN|You completed all 5 rounds!\n");
        }
    } else {
        // Lose - show solution
        room_broadcast(server, room_id,
            "GAME_END|LOSE|Wrong answer!|P1[2,3]=15 + ...\n");
    }
    
    // Reset room
    room->game_started = 0;
    room->current_round = 0;
    // ...
}
```

### 8.5. Timer System

```c
// In server_run() periodic tasks:
for (int i = 0; i < MAX_ROOMS; i++) {
    if (rooms[i].active && rooms[i].game_started) {
        int elapsed = now - rooms[i].game_start_time;
        rooms[i].game_time_remaining = GAME_DURATION - elapsed;
        
        if (rooms[i].game_time_remaining <= 0) {
            // TIME'S UP!
            room_end_game(server, i, 0, 1);  // won=0, timeout=1
        } else {
            // Broadcast timer update
            char msg[64];
            sprintf(msg, "TIMER|%d\n", rooms[i].game_time_remaining);
            room_broadcast(server, i, msg, -1);
        }
    }
}
```

---

## TÓM TẮT

### Điểm Mạnh
✅ **Kiến trúc đơn giản**: Single-process, dễ hiểu, dễ debug  
✅ **select() hiệu quả**: Đủ cho game nhỏ (100 clients)  
✅ **Reconnection**: Cho phép player reconnect trong 60s  
✅ **Asymmetric info**: Core mechanic độc đáo  
✅ **Round system**: Độ khó tăng dần  
✅ **Keep-alive**: PING/PONG detection  

### Giới Hạn
⚠️ **FD_SETSIZE**: Giới hạn ~1024 connections  
⚠️ **Single-threaded**: Không tận dụng multi-core  
⚠️ **Blocking operations**: CPU-bound task block toàn server  

### Cải Tiến Có Thể
- Chuyển sang `epoll` (Linux) hoặc `kqueue` (BSD) để scale tốt hơn
- Multi-threading cho game logic
- Database thay vì file text
- Encryption cho passwords
- TLS/SSL cho network

---

**Document Version**: 1.0  
**Last Updated**: 2025-11-30  
**Author**: Math Puzzle Game Server Team

