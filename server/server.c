#include "server.h"

// Initialize server
void server_init(Server *server) {
    memset(server, 0, sizeof(Server));
    
    // Create listening socket
    server->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->listen_fd < 0) {
        perror("socket");
        exit(1);
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server->listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(1);
    }
    
    // Bind socket
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    
    if (bind(server->listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }
    
    // Listen
    if (listen(server->listen_fd, 10) < 0) {
        perror("listen");
        exit(1);
    }
    
    // Initialize fd_set
    FD_ZERO(&server->master_set);
    FD_SET(server->listen_fd, &server->master_set);
    server->max_fd = server->listen_fd;
    
    // Initialize clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        server->clients[i].active = 0;
        server->clients[i].socket_fd = -1;
    }
    
    // Initialize rooms
    for (int i = 0; i < MAX_ROOMS; i++) {
        server->rooms[i].active = 0;
        server->rooms[i].id = i;
        server->rooms[i].player_count = 0;
        server->rooms[i].game_started = 0;
    }
    
    server->last_tick_time = time(NULL);
    
    printf("Server initialized on port %d\n", PORT);
}

// Main server loop with select()
void server_run(Server *server) {
    while (1) {
        fd_set read_fds = server->master_set;
        struct timeval timeout;
        timeout.tv_sec = 1;  // Check every second
        timeout.tv_usec = 0;
        
        int activity = select(server->max_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            perror("select");
            continue;
        }
        
        // Check for new connections
        if (FD_ISSET(server->listen_fd, &read_fds)) {
            client_accept(server);
        }
        
        // Check existing clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (server->clients[i].active && FD_ISSET(server->clients[i].socket_fd, &read_fds)) {
                client_process_data(server, i);
            }
        }
        
        // Periodic tasks (every second)
        time_t now = time(NULL);
        if (now - server->last_tick_time >= 1) {
            server->last_tick_time = now;
            
            // Update game timers
            for (int i = 0; i < MAX_ROOMS; i++) {
                if (server->rooms[i].active && server->rooms[i].game_started) {
                    int elapsed = now - server->rooms[i].game_start_time;
                    server->rooms[i].game_time_remaining = GAME_DURATION - elapsed;
                    
                    if (server->rooms[i].game_time_remaining <= 0) {
                        // Time's up!
                        room_end_game(server, i, 0, 1);  // 1 = timeout
                    } else {
                        broadcast_timer_update(server, i);
                    }
                }
            }
            
            // Send room status updates every 2 seconds for active rooms (not in game)
            static time_t last_room_update = 0;
            if (now - last_room_update >= 2) {
                for (int i = 0; i < MAX_ROOMS; i++) {
                    if (server->rooms[i].active && !server->rooms[i].game_started) {
                        send_room_status(server, i);
                    }
                }
                last_room_update = now;
            }
            
            // Check ping timeouts
            check_ping_timeouts(server);
            
            // Check reconnect timeouts
            check_reconnect_timeouts(server);
            
            // Send PING every interval
            static time_t last_ping = 0;
            if (now - last_ping >= PING_INTERVAL) {
                send_ping_to_all(server);
                last_ping = now;
            }
        }
    }
}

// Accept new client connection
int client_accept(Server *server) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    int new_socket = accept(server->listen_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (new_socket < 0) {
        perror("accept");
        return -1;
    }
    
    // Find free client slot
    int client_idx = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!server->clients[i].active) {
            client_idx = i;
            break;
        }
    }
    
    if (client_idx == -1) {
        printf("Max clients reached, rejecting connection\n");
        close(new_socket);
        return -1;
    }
    
    // Initialize client
    Client *client = &server->clients[client_idx];
    memset(client, 0, sizeof(Client));
    client->socket_fd = new_socket;
    client->active = 1;
    client->state = STATE_CONNECTED;
    client->room_id = -1;
    client->player_index = -1;
    client->last_pong_time = time(NULL);
    client->last_ping_time = time(NULL);
    
    // Add to fd_set
    FD_SET(new_socket, &server->master_set);
    if (new_socket > server->max_fd) {
        server->max_fd = new_socket;
    }
    
    printf("New client connected: %s:%d (socket %d, index %d)\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),
           new_socket, client_idx);
    
    client_send(client, "WELCOME|Math Puzzle Game Server v1.0\n");
    
    return client_idx;
}

// Disconnect client
// Mark client as disconnected (allow reconnect)
void client_mark_disconnected(Server *server, int client_idx) {
    Client *client = &server->clients[client_idx];
    
    if (!client->active) return;
    
    printf("Client disconnected: %s (socket %d), allowing reconnect...\n", 
           client->username[0] ? client->username : "unknown", 
           client->socket_fd);
    
    // Save state before disconnect
    client->saved_state = client->state;
    client->state = STATE_DISCONNECTED;
    client->disconnect_time = time(NULL);
    
    // Close socket but keep client data
    FD_CLR(client->socket_fd, &server->master_set);
    close(client->socket_fd);
    client->socket_fd = -1;
    
    // If in a room, notify other players (but don't remove yet)
    if (client->room_id >= 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "PLAYER_DISCONNECTED|%s\n", client->username);
        room_broadcast(server, client->room_id, msg, client_idx);
        
        printf("Player %s in room %d marked as disconnected. Waiting for reconnect...\n",
               client->username, client->room_id);
    }
}

// Permanently disconnect client (cleanup)
void client_disconnect(Server *server, int client_idx) {
    Client *client = &server->clients[client_idx];
    
    if (!client->active) return;
    
    printf("Permanently disconnecting client: %s\n", 
           client->username[0] ? client->username : "unknown");
    
    // If in a room, handle room cleanup
    if (client->room_id >= 0) {
        Room *room = &server->rooms[client->room_id];
        
        // Notify other players
        char msg[256];
        snprintf(msg, sizeof(msg), "PLAYER_LEFT|%s\n", client->username);
        room_broadcast(server, client->room_id, msg, client_idx);
        
        // If game in progress, abort it
        if (room->game_started) {
            room_broadcast(server, client->room_id, "GAME_ABORTED|Player disconnected\n", -1);
        }
        
        // Remove player from room
        for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
            if (room->player_ids[i] == client_idx) {
                room->player_ids[i] = -1;
                room->player_ready[i] = 0;
                break;
            }
        }
        room->player_count--;
        
        // Clean up room if empty
        if (room->player_count == 0) {
            room_cleanup(server, client->room_id);
        }
    }
    
    // Remove from fd_set if still open
    if (client->socket_fd >= 0) {
        FD_CLR(client->socket_fd, &server->master_set);
        close(client->socket_fd);
    }
    
    // Clear client data
    client->active = 0;
    client->state = STATE_CONNECTED;
    client->room_id = -1;
}

// Process incoming data from client (Stream processing with buffer)
void client_process_data(Server *server, int client_idx) {
    Client *client = &server->clients[client_idx];
    
    char temp_buf[BUFFER_SIZE];
    int bytes_read = recv(client->socket_fd, temp_buf, sizeof(temp_buf) - 1, 0);
    
    if (bytes_read <= 0) {
        // Connection closed or error - mark as disconnected (allow reconnect)
        client_mark_disconnected(server, client_idx);
        return;
    }
    
    temp_buf[bytes_read] = '\0';
    
    // Append to client's buffer
    int space_left = BUFFER_SIZE - client->buffer_len - 1;
    if (bytes_read > space_left) {
        printf("Buffer overflow for client %d, clearing buffer\n", client_idx);
        client->buffer_len = 0;
    }
    
    memcpy(client->recv_buffer + client->buffer_len, temp_buf, bytes_read);
    client->buffer_len += bytes_read;
    client->recv_buffer[client->buffer_len] = '\0';
    
    // Process complete messages (delimited by \n)
    char *line_start = client->recv_buffer;
    char *line_end;
    
    while ((line_end = strchr(line_start, '\n')) != NULL) {
        *line_end = '\0';  // Null-terminate the line
        
        // Process the message
        if (strlen(line_start) > 0) {
            handle_message(server, client_idx, line_start);
        }
        
        line_start = line_end + 1;
    }
    
    // Move remaining data to beginning of buffer
    int remaining = strlen(line_start);
    if (remaining > 0) {
        memmove(client->recv_buffer, line_start, remaining);
    }
    client->buffer_len = remaining;
    client->recv_buffer[client->buffer_len] = '\0';
}

// Send message to client
void client_send(Client *client, const char *message) {
    if (!client->active) return;
    
    int len = strlen(message);
    int sent = send(client->socket_fd, message, len, 0);
    
    if (sent < 0) {
        perror("send");
    }
}

// Handle incoming message
void handle_message(Server *server, int client_idx, const char *message) {
    Client *client = &server->clients[client_idx];
    
    printf("Received from %s: %s\n", 
           client->username[0] ? client->username : "unknown", 
           message);
    
    // Parse command
    char cmd[64] = {0};
    char arg1[256] = {0};
    char arg2[256] = {0};
    
    sscanf(message, "%63[^|]|%255[^|]|%255s", cmd, arg1, arg2);
    
    if (strcmp(cmd, "REGISTER") == 0) {
        handle_register(server, client_idx, arg1, arg2);
    }
    else if (strcmp(cmd, "LOGIN") == 0) {
        handle_login(server, client_idx, arg1, arg2);
    }
    else if (strcmp(cmd, "CREATE_ROOM") == 0) {
        handle_create_room(server, client_idx, arg1);
    }
    else if (strcmp(cmd, "JOIN_ROOM") == 0) {
        int room_id = atoi(arg1);
        handle_join_room(server, client_idx, room_id);
    }
    else if (strcmp(cmd, "LEAVE_ROOM") == 0) {
        handle_leave_room(server, client_idx);
    }
    else if (strcmp(cmd, "LIST_ROOMS") == 0) {
        send_room_list(server, client_idx);
    }
    else if (strcmp(cmd, "READY") == 0) {
        handle_ready(server, client_idx);
    }
    else if (strcmp(cmd, "START_GAME") == 0) {
        handle_start_game(server, client_idx);
    }
    else if (strcmp(cmd, "SUBMIT") == 0) {
        // Format: SUBMIT|row|col
        int row = atoi(arg1);
        int col = atoi(arg2);
        handle_submit(server, client_idx, row, col);
    }
    else if (strcmp(cmd, "PONG") == 0) {
        handle_pong(server, client_idx);
    }
    else if (strcmp(cmd, "CHAT") == 0) {
        handle_chat(server, client_idx, arg1);
    }
    else {
        client_send(client, "ERROR|Unknown command\n");
    }
}

// Check for disconnected clients that exceed reconnect timeout
void check_reconnect_timeouts(Server *server) {
    time_t now = time(NULL);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        Client *client = &server->clients[i];
        
        if (client->active && client->state == STATE_DISCONNECTED) {
            time_t elapsed = now - client->disconnect_time;
            
            if (elapsed >= RECONNECT_TIMEOUT) {
                printf("Reconnect timeout for %s, permanently disconnecting...\n", client->username);
                
                // Permanently disconnect
                client_disconnect(server, i);
            }
        }
    }
}

// Main function
int main() {
    srand(time(NULL));
    
    Server server;
    server_init(&server);
    
    printf("Math Puzzle Game Server running...\n");
    printf("Waiting for players...\n\n");
    
    server_run(&server);
    
    return 0;
}

