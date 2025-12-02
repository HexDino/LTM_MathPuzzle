#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

#define PORT 8888
#define MAX_CLIENTS 100
#define MAX_ROOMS 25
#define PLAYERS_PER_ROOM 4
#define BUFFER_SIZE 4096
#define MAX_USERNAME 32
#define MAX_PASSWORD 64
#define MAX_ROOM_NAME 32
#define MATRIX_SIZE 4
#define GAME_DURATION 180  // 3 minutes in seconds
#define PING_INTERVAL 10   // Send PING every 10 seconds
#define PING_TIMEOUT 30    // Disconnect if no PONG after 30 seconds
#define RECONNECT_TIMEOUT 60  // Allow reconnect within 60 seconds

// Client states
typedef enum {
    STATE_CONNECTED,
    STATE_AUTHENTICATED,
    STATE_IN_LOBBY,
    STATE_IN_ROOM,
    STATE_READY,
    STATE_IN_GAME,
    STATE_DISCONNECTED  // Temporarily disconnected, can reconnect
} ClientState;

// Game operators
typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV
} Operator;

// Equation format types
typedef enum {
    FORMAT_P1_P2_P3_EQ_P4,   // P1 ± P2 ± P3 = P4
    FORMAT_P1_EQ_P2_P3_P4,   // P1 = P2 ± P3 ± P4
    FORMAT_P1_P2_EQ_P3_P4    // P1 ± P2 = P3 ± P4
} EquationFormat;

// Matrix structure
typedef struct {
    int data[MATRIX_SIZE][MATRIX_SIZE];
} Matrix;

// Puzzle structure
typedef struct {
    Operator op1;  // Between P1 and P2
    Operator op2;  // Between P2/P3 and P3/P4 (depends on format)
    Operator op3;  // For 3-operator formats
    EquationFormat format;
    Matrix matrices[PLAYERS_PER_ROOM];
    int solution_row[PLAYERS_PER_ROOM];
    int solution_col[PLAYERS_PER_ROOM];
    int solution_values[PLAYERS_PER_ROOM];
    int result;
    int round;  // Current round (1-5)
} Puzzle;

// Room structure
typedef struct {
    int id;
    char name[MAX_ROOM_NAME];
    int active;
    int player_ids[PLAYERS_PER_ROOM];
    int player_ready[PLAYERS_PER_ROOM];
    int player_count;
    int game_started;
    int host_index;  // Index of the host player (0-3), who created the room
    Puzzle puzzle;
    time_t game_start_time;
    int game_time_remaining;
    int submitted_answers[PLAYERS_PER_ROOM][2];  // [row, col]
    int answer_submitted[PLAYERS_PER_ROOM];
    int all_submitted;
    int current_round;  // Current round (1-5)
    int total_rounds;   // Total rounds to win (default 5)
} Room;

// Client structure
typedef struct {
    int socket_fd;
    int active;
    char username[MAX_USERNAME];
    char recv_buffer[BUFFER_SIZE];
    int buffer_len;
    ClientState state;
    int room_id;
    int player_index;  // 0-3 in room
    time_t last_pong_time;
    time_t last_ping_time;
    int ping_ms;  // Stored RTT in milliseconds
    time_t disconnect_time;  // Time when client disconnected
    ClientState saved_state;  // State before disconnect
} Client;

// Server state
typedef struct {
    int listen_fd;
    Client clients[MAX_CLIENTS];
    Room rooms[MAX_ROOMS];
    fd_set master_set;
    int max_fd;
    time_t last_tick_time;
} Server;

// Function declarations

// Server management
void server_init(Server *server);
void server_run(Server *server);
void server_shutdown(Server *server);

// Client management
int client_accept(Server *server);
void client_disconnect(Server *server, int client_idx);
void client_mark_disconnected(Server *server, int client_idx);
void check_reconnect_timeouts(Server *server);
void client_process_data(Server *server, int client_idx);
void client_send(Client *client, const char *message);

// Protocol handling
void handle_message(Server *server, int client_idx, const char *message);
void handle_register(Server *server, int client_idx, const char *username, const char *password);
void handle_login(Server *server, int client_idx, const char *username, const char *password);
void handle_create_room(Server *server, int client_idx, const char *room_name);
void handle_join_room(Server *server, int client_idx, int room_id);
void handle_leave_room(Server *server, int client_idx);
void handle_ready(Server *server, int client_idx);
void handle_start_game(Server *server, int client_idx);
void handle_submit(Server *server, int client_idx, int row, int col);
void handle_pong(Server *server, int client_idx);
void handle_chat(Server *server, int client_idx, const char *message);

// Room management
int room_create(Server *server, const char *name);
int room_join(Server *server, int room_id, int client_idx);
void room_start_game(Server *server, int room_id);
void room_end_game(Server *server, int room_id, int won, int timeout);
void room_broadcast(Server *server, int room_id, const char *message, int exclude_client_idx);
void room_cleanup(Server *server, int room_id);

// Game logic
void puzzle_generate(Puzzle *puzzle, int round);
void puzzle_send_to_clients(Server *server, int room_id);
int puzzle_verify_solution(Puzzle *puzzle, int submitted[PLAYERS_PER_ROOM][2]);

// Utility functions
void send_room_list(Server *server, int client_idx);
void send_room_status(Server *server, int room_id);
void broadcast_timer_update(Server *server, int room_id);
void check_ping_timeouts(Server *server);
void send_ping_to_all(Server *server);
int authenticate_user(const char *username, const char *password);
int register_user(const char *username, const char *password);
char* get_operator_string(Operator op);
int calculate_result(int p1, Operator op1, int p2, Operator op2, int p3);

#endif // SERVER_H

