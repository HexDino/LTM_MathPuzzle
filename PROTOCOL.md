# Math Puzzle Game - Protocol

## Format cơ bản

```
COMMAND|arg1|arg2|...\n
```

- Commands viết HOA
- Delimiter: dấu `|`
- Kết thúc: `\n`

## Commands chính

### Authentication
```
REGISTER|username|password
LOGIN|username|password
```

### Lobby
```
LIST_ROOMS
CREATE_ROOM|roomName
JOIN_ROOM|roomId
```

### Game
```
READY
CHAT|message
SUBMIT|row|col
PONG
```

## Server Responses

### Connection
```
WELCOME|Math Puzzle Game Server v1.0
LOGIN_OK|username
ROOM_LIST|id:name:count|id:name:count|...
```

### Room
```
ROOM_CREATED|roomId|roomName
ROOM_JOINED|roomId
PLAYER_JOINED|playerIndex|username
ROOM_STATUS|count|idx:name:ready|idx:name:ready|...
```

### Game
```
GAME_START|equation|matrix0|matrix1|matrix2|matrix3
TIMER|seconds
PLAYER_SUBMITTED|playerIndex|username
GAME_END|WIN|message
GAME_END|LOSE|solution
GAME_ABORTED|reason
```

### System
```
PING
ERROR|message
```

## Game Start Format

```
GAME_START|P1+P2*P3=P4|HIDDEN|1,2,3,...,16|10,20,...|5,10,...
```

- `equation`: Phương trình cần giải
- Mỗi ma trận: 16 số (4x4) hoặc `HIDDEN`
- Mỗi player nhận HIDDEN cho ma trận của mình

## Example Session

```
# Client kết nối
← WELCOME|...

# Đăng nhập
→ LOGIN|alice|pass123
← LOGIN_OK|alice
← ROOM_LIST|...

# Tạo phòng
→ CREATE_ROOM|My Room
← ROOM_CREATED|0|My Room
← ROOM_STATUS|1|0:alice:0

# 3 người khác join...
← PLAYER_JOINED|1|bob
← ROOM_STATUS|2|...

# Cả 4 người ready
→ READY
← ROOM_STATUS|4|0:alice:1|1:bob:1|2:charlie:1|3:diana:1
← GAME_START|P1-P2*P3=P4|HIDDEN|...|...|...
← TIMER|180

# Chơi game
→ CHAT|My row 0 is: 5,10,15,20
← CHAT|alice|My row 0 is: 5,10,15,20
← TIMER|179

→ SUBMIT|2|3
← PLAYER_SUBMITTED|0|alice

# Kết thúc
← GAME_END|WIN|Congratulations!
```

## State Flow

```
CONNECTED → (LOGIN) → IN_LOBBY → (JOIN_ROOM) → IN_ROOM 
→ (READY) → IN_GAME → (GAME_END) → IN_ROOM
```

## Lưu ý

- Client tự động PONG khi nhận PING (timeout 30s)
- Ma trận format: 16 số ngăn cách bởi dấu phẩy
- Tọa độ ma trận: 0-3 (4x4)
- Thời gian game: 180 giây

