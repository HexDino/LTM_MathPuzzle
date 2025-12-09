# BÁO CÁO KỸ THUẬT - PHẦN SERVER
## Math Puzzle Game - Network Programming Project

---

## MỤC LỤC

1. [Thiết lập I/O Multiplexing Socket cho Server](#1-thiết-lập-io-multiplexing-socket-cho-server)
2. [Xử lý liên lạc thông tin với Protocol tự thiết kế](#2-xử-lý-liên-lạc-thông-tin-với-protocol-tự-thiết-kế)
3. [Quản lý phòng chơi của Client tạo](#3-quản-lý-phòng-chơi-của-client-tạo)
4. [Xử lý mất kết nối với Client (Ping/Pong)](#4-xử-lý-mất-kết-nối-với-client-pingpong)
5. [Quản lý đồng bộ thời gian chơi game](#5-quản-lý-đồng-bộ-thời-gian-chơi-game)
6. [Thiết lập cơ chế Logic game](#6-thiết-lập-cơ-chế-logic-game)
7. [Chức năng Chat trong phòng](#7-chức-năng-chat-trong-phòng)
8. [Tổng kết](#8-tổng-kết)

---

## 1. THIẾT LẬP I/O MULTIPLEXING SOCKET CHO SERVER

### 1.1. Tổng quan

**I/O Multiplexing** cho phép server theo dõi nhiều socket đồng thời trong một thread duy nhất, xử lý hiệu quả nhiều client mà không cần tạo thread riêng cho mỗi kết nối.

**Lựa chọn kỹ thuật:** Sử dụng `select()` - cross-platform, phù hợp cho quy mô game (max 100 clients).

### 1.2. Khởi tạo Server

```c
void server_init(Server *server) {
    // 1. Tạo TCP socket
    server->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // 2. Set SO_REUSEADDR để bind lại ngay sau khi tắt
    int opt = 1;
    setsockopt(server->listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 3. Bind và Listen trên port 8888
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8888);
    bind(server->listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server->listen_fd, 10);
    
    // 4. Khởi tạo fd_set cho select()
    FD_ZERO(&server->master_set);
    FD_SET(server->listen_fd, &server->master_set);
    server->max_fd = server->listen_fd;
}
```

**Các bước chính:**
- Tạo listening socket với TCP (SOCK_STREAM)
- Set SO_REUSEADDR để tránh "Address already in use" sau khi restart
- Bind vào INADDR_ANY (mọi network interfaces) port 8888
- Khởi tạo fd_set để track tất cả sockets cần monitor

### 1.3. Event Loop với select()

```c
void server_run(Server *server) {
    while (1) {
        fd_set read_fds = server->master_set;  // Copy master set
        struct timeval timeout = {1, 0};        // Timeout 1 giây
        
        // Chờ sự kiện I/O
        int activity = select(server->max_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        // Xử lý new connection
        if (FD_ISSET(server->listen_fd, &read_fds)) {
            client_accept(server);
        }
        
        // Xử lý data từ existing clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (server->clients[i].active && 
                FD_ISSET(server->clients[i].socket_fd, &read_fds)) {
                client_process_data(server, i);
            }
        }
        
        // Periodic tasks (mỗi giây: timer update, ping/pong check)
        if (time(NULL) - server->last_tick_time >= 1) {
            server->last_tick_time = time(NULL);
            update_game_timers(server);
            check_ping_timeouts(server);
            send_ping_to_all(server);
        }
    }
}
```

**Luồng xử lý:**
```
select() → New connection? → accept() + add to master_set
         → Client has data? → recv() + parse message
         → Timeout?        → Update timers, ping/pong
```

**Ưu điểm:**
- Single-threaded: Không cần locking/synchronization
- Event-driven: Chỉ xử lý khi có sự kiện
- Hiệu quả: CPU idle khi không có I/O

---

## 2. XỬ LÝ LIÊN LẠC THÔNG TIN VỚI PROTOCOL TỰ THIẾT KẾ

### 2.1. Thiết kế Protocol

**Format:** Text-based, line-delimited với delimiter `|`
- Cấu trúc: `COMMAND|arg1|arg2|...\n`
- Ưu điểm: Human-readable, dễ debug, cross-platform

**Các message chính:**

| Direction | Message | Format | Mô tả |
|-----------|---------|--------|-------|
| C→S | LOGIN | `LOGIN\|username\|password` | Đăng nhập |
| C→S | CREATE_ROOM | `CREATE_ROOM\|room_name` | Tạo phòng |
| C→S | JOIN_ROOM | `JOIN_ROOM\|room_id` | Tham gia phòng |
| C→S | READY | `READY` | Sẵn sàng chơi |
| C→S | START_GAME | `START_GAME` | Bắt đầu (host) |
| C→S | SUBMIT | `SUBMIT\|row\|col` | Nộp đáp án |
| C→S | CHAT | `CHAT\|message` | Chat |
| C→S | PONG | `PONG` | Phản hồi ping |
| S→C | ROOM_STATUS | `ROOM_STATUS\|count\|host\|players` | Trạng thái phòng |
| S→C | GAME_START | `GAME_START\|equation\|matrices` | Bắt đầu game |
| S→C | TIMER | `TIMER\|seconds` | Cập nhật timer |
| S→C | PING | `PING` | Keep-alive |

### 2.2. Xử lý Stream với Buffer

**Vấn đề:** TCP là stream protocol, message có thể bị tách hoặc ghép.

```c
void client_process_data(Server *server, int client_idx) {
    Client *client = &server->clients[client_idx];
    char temp_buf[BUFFER_SIZE];
    int bytes_read = recv(client->socket_fd, temp_buf, sizeof(temp_buf)-1, 0);
    
    if (bytes_read <= 0) {
        client_mark_disconnected(server, client_idx);
        return;
    }
    
    // Append vào buffer
    memcpy(client->recv_buffer + client->buffer_len, temp_buf, bytes_read);
    client->buffer_len += bytes_read;
    
    // Parse complete messages (delimited by \n)
    char *line_start = client->recv_buffer;
    char *line_end;
    while ((line_end = strchr(line_start, '\n')) != NULL) {
        *line_end = '\0';
        handle_message(server, client_idx, line_start);
        line_start = line_end + 1;
    }
    
    // Move remaining data to buffer start
    int remaining = strlen(line_start);
    if (remaining > 0) memmove(client->recv_buffer, line_start, remaining);
    client->buffer_len = remaining;
}
```

**Giải pháp:** 
- Buffer tích lũy data từ recv()
- Parse khi gặp delimiter `\n`
- Giữ lại partial message cho lần recv() tiếp theo

### 2.3. Command Router

```c
void handle_message(Server *server, int client_idx, const char *message) {
    char cmd[64], arg1[256], arg2[256];
    sscanf(message, "%63[^|]|%255[^|]|%255s", cmd, arg1, arg2);
    
    if (strcmp(cmd, "LOGIN") == 0)
        handle_login(server, client_idx, arg1, arg2);
    else if (strcmp(cmd, "CREATE_ROOM") == 0)
        handle_create_room(server, client_idx, arg1);
    else if (strcmp(cmd, "SUBMIT") == 0)
        handle_submit(server, client_idx, atoi(arg1), atoi(arg2));
    else if (strcmp(cmd, "PONG") == 0)
        handle_pong(server, client_idx);
    // ... các commands khác
}
```

**Pattern:** Parse command → Dispatch đến handler tương ứng

---

## 3. QUẢN LÝ PHÒNG CHƠI CỦA CLIENT TẠO

### 3.1. Cấu trúc dữ liệu Room

```c
typedef struct {
    int id;                                   // Room ID (0-24)
    char name[MAX_ROOM_NAME];                 // Tên phòng
    int active;                               // Active flag
    int player_ids[PLAYERS_PER_ROOM];        // Mảng client indices (4 players)
    int player_ready[PLAYERS_PER_ROOM];      // Ready status
    int player_count;                         // Số player hiện tại
    int host_index;                           // Index của host (player đầu tiên)
    int game_started;                         // Game state
    Puzzle puzzle;                            // Puzzle hiện tại
    time_t game_start_time;                   // Thời gian bắt đầu
    int game_time_remaining;                  // Countdown timer
    int current_round, total_rounds;          // Round progression (1-5)
} Room;
```

Server quản lý tối đa 25 phòng, mỗi phòng chứa tối đa 4 players.

### 3.2. Tạo và tham gia phòng

**Create Room:**
```c
int room_create(Server *server, const char *name) {
    // Tìm slot trống trong MAX_ROOMS (25 phòng)
    int room_idx = find_free_room_slot(server);
    if (room_idx == -1) return -1;
    
    // Khởi tạo room và auto-join creator
    Room *room = &server->rooms[room_idx];
    memset(room, 0, sizeof(Room));
    room->id = room_idx;
    room->active = 1;
    strncpy(room->name, name, MAX_ROOM_NAME - 1);
    return room_idx;
}
```

**Join Room:**
```c
int room_join(Server *server, int room_id, int client_idx) {
    Room *room = &server->rooms[room_id];
    
    // Validate: room exists, not full, game not started
    if (!room->active || room->player_count >= 4 || room->game_started)
        return 0;
    
    // Tìm slot trống và add player
    int slot = find_free_player_slot(room);
    room->player_ids[slot] = client_idx;
    room->player_count++;
    
    // Set host nếu là player đầu tiên
    if (room->host_index == -1)
        room->host_index = slot;
    
    // Update client state và notify all
    server->clients[client_idx].room_id = room_id;
    server->clients[client_idx].state = STATE_IN_ROOM;
    room_broadcast(server, room_id, "PLAYER_JOINED|...\n", -1);
    send_room_status(server, room_id);
    
    return 1;
}
```

### 3.3. Host privileges và Leave room

**Chỉ host có quyền START_GAME:**
```c
void handle_start_game(Server *server, int client_idx) {
    Client *client = &server->clients[client_idx];
    Room *room = &server->rooms[client->room_id];
    
    // Verify host privilege
    if (client->player_index != room->host_index) {
        client_send(client, "ERROR|Only host can start\n");
        return;
    }
    
    room_start_game(server, client->room_id);
}
```

**Leave Room:** 
- Notify players
- Remove player khỏi room
- **Transfer host** nếu host leave (assign sang player khác)
- **Cleanup room** nếu empty
- **Abort game** nếu đang chơi

### 3.4. Room Status Broadcasting

Server gửi cập nhật room status định kỳ (mỗi 2 giây) và khi có sự kiện:

```c
void send_room_status(Server *server, int room_id) {
    // Format: ROOM_STATUS|count|host_idx|p0:name:ready:ping|p1:...|...
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "ROOM_STATUS|%d|%d",
            room->player_count, room->host_index);
    
    for (int i = 0; i < 4; i++) {
        if (room->player_ids[i] >= 0) {
            Client *c = &server->clients[room->player_ids[i]];
            // Append: |index:username:ready:ping_ms
            strcat_format(buffer, "|%d:%s:%d:%d", 
                         i, c->username, room->player_ready[i], c->ping_ms);
        }
    }
    
    room_broadcast(server, room_id, buffer, -1);
}
```

---

## 4. XỬ LÝ MẤT KẾT NỐI VỚI CLIENT (PING/PONG)

### 4.1. Keep-Alive Mechanism

**Vấn đề:** TCP không tự động phát hiện client disconnect (mất mạng, đóng app đột ngột).

**Giải pháp:** Heartbeat với PING/PONG

```
Server ──PING (mỗi 10s)──▶ Client
Server ◀────PONG────────── Client

Không nhận PONG sau 30s → Disconnect
```

### 4.2. Implementation

```c
#define PING_INTERVAL 10    // Gửi PING mỗi 10 giây
#define PING_TIMEOUT 30     // Timeout sau 30 giây
#define RECONNECT_TIMEOUT 60 // Cho phép reconnect trong 60 giây

// Gửi PING định kỳ (trong main loop)
void send_ping_to_all(Server *server) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].active) {
            server->clients[i].last_ping_time = time(NULL);
            client_send(&server->clients[i], "PING\n");
        }
    }
}

// Xử lý PONG - tính RTT (Round Trip Time)
void handle_pong(Server *server, int client_idx) {
    Client *client = &server->clients[client_idx];
    time_t now = time(NULL);
    
    // RTT = thời gian nhận PONG - thời gian gửi PING
    client->ping_ms = (int)((now - client->last_ping_time) * 1000);
    client->last_pong_time = now;
}

// Check timeout
void check_ping_timeouts(Server *server) {
    time_t now = time(NULL);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].active &&
            now - server->clients[i].last_pong_time > PING_TIMEOUT) {
            client_mark_disconnected(server, i);
        }
    }
}
```

**Tính năng ping display:**
- Tính RTT khi nhận PONG (milliseconds)
- Gửi ping_ms trong ROOM_STATUS để client hiển thị
- Color coding: Green (<50ms), Orange (50-100ms), Red (>100ms)

### 4.3. Temporary Disconnect & Reconnection

**Khi detect timeout:** Mark as disconnected nhưng giữ state trong 60 giây

```c
void client_mark_disconnected(Server *server, int client_idx) {
    Client *client = &server->clients[client_idx];
    
    // Lưu state để restore khi reconnect
    client->saved_state = client->state;
    client->state = STATE_DISCONNECTED;
    client->disconnect_time = time(NULL);
    
    // Close socket nhưng GIỮ client data (room_id, username, etc.)
    FD_CLR(client->socket_fd, &server->master_set);
    close(client->socket_fd);
    client->socket_fd = -1;
    
    // Notify room nhưng không remove player
    if (client->room_id >= 0) {
        room_broadcast(server, client->room_id, 
                      "PLAYER_DISCONNECTED|...\n", -1);
    }
}
```

**Khi login lại trong 60s:** 
- Check username có session đang disconnect không
- Restore room_id, player_index, saved_state
- Update room's player_ids mapping
- Broadcast PLAYER_RECONNECTED

**Sau 60s:** Permanent disconnect → remove khỏi room, cleanup

---

## 5. QUẢN LÝ ĐỒNG BỘ THỜI GIAN CHƠI GAME

### 5.1. Server-Authoritative Timer

**Thiết kế:** Server là source of truth duy nhất, tránh clock skew giữa clients.

```c
#define GAME_DURATION 180  // 3 phút

void room_start_game(Server *server, int room_id) {
    Room *room = &server->rooms[room_id];
    
    // Lưu timestamp bắt đầu
    room->game_start_time = time(NULL);
    room->game_time_remaining = GAME_DURATION;
    room->game_started = 1;
    
    // Generate và send puzzle...
}
```

### 5.2. Timer Update trong Event Loop

**Mỗi giây:** Tính elapsed time và broadcast

```c
void server_run(Server *server) {
    while (1) {
        // ... select() ...
        
        // Periodic tasks (mỗi 1 giây)
        if (time(NULL) - server->last_tick_time >= 1) {
            for (int i = 0; i < MAX_ROOMS; i++) {
                Room *room = &server->rooms[i];
                if (room->active && room->game_started) {
                    // Tính thời gian còn lại
                    int elapsed = time(NULL) - room->game_start_time;
                    room->game_time_remaining = GAME_DURATION - elapsed;
                    
                    if (room->game_time_remaining <= 0) {
                        room_end_game(server, i, 0, 1);  // Timeout
                    } else {
                        // Broadcast: TIMER|179\n
                        broadcast_timer_update(server, i);
                    }
                }
            }
        }
    }
}
```

**Broadcast timer:**
```c
void broadcast_timer_update(Server *server, int room_id) {
    Room *room = &server->rooms[room_id];
    char msg[64];
    snprintf(msg, sizeof(msg), "TIMER|%d\n", room->game_time_remaining);
    room_broadcast(server, room_id, msg, -1);
}
```

### 5.3. Timeout Handling

```c
void room_end_game(Server *server, int room_id, int won, int timeout) {
    Room *room = &server->rooms[room_id];
    
    if (timeout) {
        // Hết giờ → Show solution
        room_broadcast(server, room_id, 
                      "GAME_END|LOSE|Time's up!|Solution:...\n", -1);
    } else if (won && room->current_round < 5) {
        // Next round
        room->current_round++;
        room_start_game(server, room_id);
        return;
    }
    
    // Reset room state
    room->game_started = 0;
    room->current_round = 0;
}
```

**Ưu điểm:**
- Đồng bộ hoàn hảo: Tất cả clients nhận cùng countdown
- Không có drift hay desync
- Server control hoàn toàn

---

## 6. THIẾT LẬP CƠ CHẾ LOGIC GAME

### 6.1. Game Flow

```
1. Host nhấn START_GAME
2. Server generate puzzle (difficulty scale theo round)
3. Send asymmetric info (mỗi player ẩn 1 matrix)
4. Players chat + submit coordinates
5. Verify: all 4 coordinates phải đúng
6. Win → Next round (5 rounds total) | Lose/Timeout → Game over
```

### 6.2. Puzzle Generation với Difficulty Scaling

```c
void puzzle_generate(Puzzle *puzzle, int round) {
    // Round 1: P1 + P2 - P3 = P4 (simple, values 1-50)
    // Round 2: P1 + P2 = P3 - P4 (medium, values 10-80)
    // Round 3: P1 = P2 * P3 + P4 (hard, multiplication)
    // Round 4: P1 * P2 = P3 * P4 (very hard)
    // Round 5: P1 * P2 / P3 = P4 (expert, negative numbers)
    
    switch (round) {
        case 1:
            puzzle->format = FORMAT_P1_P2_P3_EQ_P4;
            puzzle->op1 = rand() % 2;  // + or -
            puzzle->op2 = rand() % 2;
            min_val = 1; max_val = 50;
            break;
        // ... cases 2-5 với difficulty tăng dần
    }
    
    // Generate 4 matrices 4x4 với random values
    for (int m = 0; m < 4; m++) {
        fill_matrix_random(puzzle->matrices[m], min_val, max_val);
        
        // Place solution values tại random positions
        puzzle->solution_row[m] = rand() % 4;
        puzzle->solution_col[m] = rand() % 4;
        puzzle->matrices[m].data[row][col] = puzzle->solution_values[m];
    }
}
```

**Progression:** Difficulty tăng dần qua 5 rounds với operations phức tạp hơn và range values rộng hơn.

### 6.3. Asymmetric Information (Core Mechanic)

**Mỗi player nhận 3/4 ma trận:**

```c
void puzzle_send_to_clients(Server *server, int room_id) {
    Room *room = &server->rooms[room_id];
    
    for (int player = 0; player < 4; player++) {
        char buffer[BUFFER_SIZE * 2];
        
        // Build: GAME_START|equation|M0|M1|M2|M3|round|total
        build_equation_string(buffer, &room->puzzle);
        
        // Với matrix của player này = "HIDDEN"
        for (int m = 0; m < 4; m++) {
            if (m == player) {
                strcat(buffer, "|HIDDEN");
            } else {
                strcat(buffer, "|");
                serialize_matrix(buffer, &room->puzzle.matrices[m]);
            }
        }
        
        // Add round info
        sprintf(buffer + strlen(buffer), "|%d|%d\n", 
                room->current_round, room->total_rounds);
        
        // Send riêng cho từng player
        client_send(&server->clients[room->player_ids[player]], buffer);
    }
}
```

**Ví dụ:** Equation `P1 + P2 - P3 = P4`
```
Player 0 nhận: [HIDDEN] [M1] [M2] [M3]
Player 1 nhận: [M0] [HIDDEN] [M2] [M3]
Player 2 nhận: [M0] [M1] [HIDDEN] [M3]
Player 3 nhận: [M0] [M1] [M2] [HIDDEN]
```

→ **Player phải chat để chia sẻ thông tin và tìm solution**

### 6.4. Answer Submission & Verification

```c
void handle_submit(Server *server, int client_idx, int row, int col) {
    Room *room = &server->rooms[client->room_id];
    int player_idx = client->player_index;
    
    // Validate coordinates
    if (row < 0 || row >= 4 || col < 0 || col >= 4) {
        client_send(client, "ERROR|Invalid coordinates\n");
        return;
    }
    
    // Lưu answer
    room->submitted_answers[player_idx][0] = row;
    room->submitted_answers[player_idx][1] = col;
    room->answer_submitted[player_idx] = 1;
    
    // Broadcast: PLAYER_SUBMITTED|player_idx|username
    room_broadcast(server, room_id, "PLAYER_SUBMITTED|...\n", -1);
    
    // Nếu tất cả đã submit → Verify
    if (all_players_submitted(room)) {
        int correct = puzzle_verify_solution(&room->puzzle, 
                                             room->submitted_answers);
        room_end_game(server, room_id, correct, 0);
    }
}

int puzzle_verify_solution(Puzzle *puzzle, int submitted[4][2]) {
    // Verify tất cả 4 coordinates
    for (int i = 0; i < 4; i++) {
        if (submitted[i][0] != puzzle->solution_row[i] ||
            submitted[i][1] != puzzle->solution_col[i])
            return 0;  // WRONG - 1 player sai → tất cả sai
    }
    return 1;  // CORRECT - teamwork success!
}
```

**Logic verification:**
- **Tất cả 4 coordinates phải đúng**
- 1 player sai → toàn bộ fail
- → Khuyến khích teamwork và communication

### 6.5. Round Progression (5 Rounds)

```c
void room_end_game(Server *server, int room_id, int won, int timeout) {
    Room *room = &server->rooms[room_id];
    
    if (won && room->current_round < 5) {
        // Next round
        room_broadcast(server, room_id, 
                      "GAME_END|WIN|Round complete! Starting next...\n", -1);
        room->current_round++;
        room_start_game(server, room_id);  // Recursive call
        return;
    } 
    else if (won && room->current_round == 5) {
        // Victory!
        room_broadcast(server, room_id, 
                      "GAME_END|WIN|Completed all 5 rounds!\n", -1);
    } 
    else {
        // Lose or timeout → Show solution
        char solution[512];
        format_solution_string(solution, &room->puzzle);
        room_broadcast(server, room_id, 
                      "GAME_END|LOSE|reason|%s\n", solution);
    }
    
    // Reset room state
    room->game_started = 0;
    room->current_round = 0;
    for (int i = 0; i < 4; i++) {
        room->player_ready[i] = 0;
        if (room->player_ids[i] >= 0)
            server->clients[room->player_ids[i]].state = STATE_IN_ROOM;
    }
}
```

---

## 7. CHỨC NĂNG CHAT TRONG PHÒNG

### 7.1. Implementation

Chat là chức năng thiết yếu cho game vì players cần communicate để chia sẻ thông tin ma trận.

```c
void handle_chat(Server *server, int client_idx, const char *message) {
    Client *client = &server->clients[client_idx];
    
    // Validate: phải trong phòng mới chat được
    if (client->room_id < 0) {
        client_send(client, "ERROR|Must be in room to chat\n");
        return;
    }
    
    // Broadcast đến tất cả players trong phòng
    char msg[BUFFER_SIZE];
    snprintf(msg, sizeof(msg), "CHAT|%s|%s\n", client->username, message);
    room_broadcast(server, client->room_id, msg, -1);
}
```

### 7.2. Protocol

**Client gửi:**
```
CHAT|message_content
```

**Server broadcast đến room:**
```
CHAT|username|message_content
```

**Ví dụ:**
```
Client gửi:  CHAT|P1 is at row 2 col 3
Server → All: CHAT|Player1|P1 is at row 2 col 3
```

### 7.3. Đặc điểm

**✅ Real-time messaging**
- Tin nhắn broadcast ngay lập tức
- Latency thấp (< 100ms trong LAN)

**✅ Room-scoped**
- Chỉ players trong cùng phòng nhận được
- Không leak sang phòng khác
- Tự động cleanup khi leave room

**✅ Username tagging**
- Mỗi message hiển thị sender
- Dễ follow conversation

**✅ Simple & lightweight**
- Không lưu history
- Không có spam protection (chấp nhận được cho game nhỏ)
- Phù hợp cho real-time coordination

### 7.4. Use Cases

**Trong phòng chờ:**
- Thảo luận chiến thuật
- Chào hỏi teammates

**Trong game:**
- Chia sẻ giá trị ma trận: "P1 is 42"
- Thảo luận solution: "Try row 2, col 3"
- Coordination: "Everyone ready?"

---

## 8. TỔNG KẾT

### 8.1. Kiến trúc tổng thể

**Server Architecture:**
```
┌─────────────────────────────────────────────────┐
│               Main Event Loop                    │
│              (select() polling)                  │
└──────────────────┬──────────────────────────────┘
                   │
        ┌──────────┴──────────┐
        │                     │
    ┌───▼────┐        ┌──────▼──────┐
    │ Accept │        │   Process    │
    │  New   │        │   Client     │
    │ Client │        │   Messages   │
    └────────┘        └──────────────┘
                             │
        ┌────────────────────┼────────────────────┐
        │                    │                    │
    ┌───▼─────┐       ┌─────▼─────┐      ┌──────▼──────┐
    │  Room   │       │   Game    │      │   Ping/     │
    │ Manager │       │  Logic    │      │   Pong      │
    └─────────┘       └───────────┘      └─────────────┘
```

### 8.2. Điểm mạnh của Implementation

**✅ I/O Multiplexing với select()**
- Single-threaded, đơn giản
- Xử lý 100 clients đồng thời
- Event-driven, hiệu quả

**✅ Protocol tự thiết kế**
- Text-based, human-readable
- Dễ debug và extend
- Line-delimited với buffer handling

**✅ Quản lý phòng chơi**
- Dynamic room creation (max 25)
- Host privileges system
- Auto host transfer
- Real-time status broadcast

**✅ Ping/Pong Keep-Alive**
- Detect disconnect trong 30s
- RTT calculation cho ping display
- Reconnection support (60s window)

**✅ Timer Synchronization**
- Server-authoritative (no drift)
- Broadcast every second
- Timeout handling

**✅ Game Logic**
- 5 rounds với difficulty scaling
- Asymmetric information mechanic
- Teamwork-oriented verification
- Round progression system

**✅ Chat System**
- Real-time room-scoped messaging
- Essential cho gameplay
- Simple broadcast mechanism

### 8.3. Kỹ thuật áp dụng

| Kỹ thuật | Mô tả | Lợi ích |
|----------|-------|---------|
| **select()** | I/O Multiplexing | Xử lý nhiều clients, single-thread |
| **TCP Stream Buffering** | Buffer + delimiter parsing | Xử lý partial/concatenated messages |
| **State Machine** | Client states (Connected → Lobby → Room → Game) | Logic rõ ràng, dễ maintain |
| **Heartbeat** | PING/PONG với timeout | Detect disconnect, measure RTT |
| **Server-Authoritative** | Timer & game logic trên server | Sync hoàn hảo, anti-cheat |
| **Asymmetric Information** | Mỗi player thấy 3/4 data | Force teamwork & communication |
| **Room Broadcasting** | Broadcast messages trong phòng | Real-time updates |
| **Host Transfer** | Auto-assign new host khi leave | High availability |
| **Reconnection** | 60s grace period | Network resilience |

### 8.4. Specs

- **Language:** C
- **Network:** TCP sockets, select() multiplexing
- **Protocol:** Text-based, line-delimited
- **Port:** 8888
- **Max Clients:** 100 concurrent
- **Max Rooms:** 25 concurrent
- **Players per Room:** 4
- **Game Duration:** 180 seconds (3 minutes)
- **Rounds:** 5 với difficulty scaling
- **Ping Interval:** 10 seconds
- **Ping Timeout:** 30 seconds
- **Reconnect Timeout:** 60 seconds

### 8.5. Cải tiến có thể

**🔧 Security:**
- Hash passwords (bcrypt/argon2)
- Rate limiting
- Input validation & sanitization

**🔧 Scalability:**
- Chuyển sang epoll (Linux) hoặc kqueue (BSD)
- Database thay vì file users.txt
- Multi-threading cho game logic

**🔧 Features:**
- Private rooms với password
- Spectator mode
- Leaderboard
- Replay system
- Chat history & filters
- Admin commands

**🔧 Reliability:**
- Error recovery
- Logging system
- Health monitoring
- Auto-restart on crash

---

**Document Version:** 1.0  
**Date:** 2025-11-30  
**Project:** Math Puzzle Game - Network Programming  
**Author:** Server Development Team

