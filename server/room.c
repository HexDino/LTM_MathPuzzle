#include "server.h"

// Create a new room
int room_create(Server *server, const char *name) {
    // Find free room slot
    int room_idx = -1;
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (!server->rooms[i].active) {
            room_idx = i;
            break;
        }
    }
    
    if (room_idx == -1) {
        return -1;  // No free rooms
    }
    
    Room *room = &server->rooms[room_idx];
    memset(room, 0, sizeof(Room));
    
    room->id = room_idx;
    room->active = 1;
    strncpy(room->name, name, MAX_ROOM_NAME - 1);
    room->player_count = 0;
    room->game_started = 0;
    room->host_index = -1;  // Will be set when first player joins
    room->waiting_for_continue = 0;  // Not waiting for continue initially
    
    for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
        room->player_ids[i] = -1;
        room->player_ready[i] = 0;
        room->round_continue_ready[i] = 0;
    }
    
    printf("Room created: %s (ID: %d)\n", name, room_idx);
    return room_idx;
}

// Join a room
int room_join(Server *server, int room_id, int client_idx) {
    if (room_id < 0 || room_id >= MAX_ROOMS || !server->rooms[room_id].active) {
        return 0;  // Invalid room
    }
    
    Room *room = &server->rooms[room_id];
    Client *client = &server->clients[client_idx];
    
    if (room->player_count >= PLAYERS_PER_ROOM) {
        return 0;  // Room full
    }
    
    if (room->game_started) {
        return 0;  // Game already started
    }
    
    // Find free slot in room
    int slot = -1;
    for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
        if (room->player_ids[i] == -1) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) return 0;
    
    // Add player to room
    room->player_ids[slot] = client_idx;
    room->player_ready[slot] = 0;
    room->player_count++;
    
    // Set first player as host
    if (room->host_index == -1) {
        room->host_index = slot;
        printf("Player %s is now the host of room %d\n", client->username, room_id);
    }
    
    client->room_id = room_id;
    client->player_index = slot;
    client->state = STATE_IN_ROOM;
    
    printf("Player %s joined room %d (slot %d)\n", client->username, room_id, slot);
    
    // Notify all players in room
    char msg[256];
    snprintf(msg, sizeof(msg), "PLAYER_JOINED|%d|%s\n", slot, client->username);
    room_broadcast(server, room_id, msg, -1);
    
    // Send room status to all
    send_room_status(server, room_id);
    
    return 1;
}

// Handle create room request
void handle_create_room(Server *server, int client_idx, const char *room_name) {
    Client *client = &server->clients[client_idx];
    
    if (client->state != STATE_IN_LOBBY) {
        client_send(client, "ERROR|Must be in lobby to create room\n");
        return;
    }
    
    if (strlen(room_name) == 0) {
        client_send(client, "ERROR|Room name required\n");
        return;
    }
    
    int room_id = room_create(server, room_name);
    if (room_id < 0) {
        client_send(client, "ERROR|Could not create room\n");
        return;
    }
    
    // Auto-join the room
    if (room_join(server, room_id, client_idx)) {
        char msg[128];
        snprintf(msg, sizeof(msg), "ROOM_CREATED|%d|%s\n", room_id, room_name);
        client_send(client, msg);
        
        // Send ROOM_JOINED to trigger room screen transition
        snprintf(msg, sizeof(msg), "ROOM_JOINED|%d\n", room_id);
        client_send(client, msg);
    }
}

// Handle join room request
void handle_join_room(Server *server, int client_idx, int room_id) {
    Client *client = &server->clients[client_idx];
    
    if (client->state != STATE_IN_LOBBY) {
        client_send(client, "ERROR|Must be in lobby to join room\n");
        return;
    }
    
    if (room_join(server, room_id, client_idx)) {
        char msg[128];
        snprintf(msg, sizeof(msg), "ROOM_JOINED|%d\n", room_id);
        client_send(client, msg);
    } else {
        client_send(client, "ERROR|Could not join room\n");
    }
}

// Handle leave room request
void handle_leave_room(Server *server, int client_idx) {
    Client *client = &server->clients[client_idx];
    
    if (client->room_id < 0) {
        client_send(client, "ERROR|Not in a room\n");
        return;
    }
    
    int room_id = client->room_id;
    Room *room = &server->rooms[room_id];
    
    // Notify other players
    char msg[256];
    snprintf(msg, sizeof(msg), "PLAYER_LEFT|%s\n", client->username);
    room_broadcast(server, room_id, msg, client_idx);
    
    // If game in progress, abort it
    if (room->game_started) {
        room_broadcast(server, room_id, "GAME_ABORTED|Player left the room\n", -1);
    }
    
    // Remove player from room
    for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
        if (room->player_ids[i] == client_idx) {
            room->player_ids[i] = -1;
            room->player_ready[i] = 0;
            
            // If this was the host, assign new host
            if (room->host_index == i && room->player_count > 1) {
                // Find first available player as new host
                for (int j = 0; j < PLAYERS_PER_ROOM; j++) {
                    if (room->player_ids[j] >= 0 && j != i) {
                        room->host_index = j;
                        printf("New host for room %d: slot %d\n", room_id, j);
                        break;
                    }
                }
            }
            break;
        }
    }
    room->player_count--;
    
    // Clean up room if empty
    if (room->player_count == 0) {
        room_cleanup(server, room_id);
    } else {
        // Send updated room status to remaining players
        send_room_status(server, room_id);
    }
    
    // Set client back to lobby
    client->room_id = -1;
    client->player_index = -1;
    client->state = STATE_IN_LOBBY;
    
    // Notify client they left
    client_send(client, "LEFT_ROOM\n");
    
    // Send room list to client
    send_room_list(server, client_idx);
    
    printf("Player %s left room %d\n", client->username, room_id);
}

// Handle ready request
void handle_ready(Server *server, int client_idx) {
    Client *client = &server->clients[client_idx];
    
    if (client->state != STATE_IN_ROOM && client->state != STATE_READY) {
        client_send(client, "ERROR|Must be in room to ready\n");
        return;
    }
    
    int room_id = client->room_id;
    Room *room = &server->rooms[room_id];
    
    // Toggle ready state
    int slot = client->player_index;
    room->player_ready[slot] = !room->player_ready[slot];
    
    if (room->player_ready[slot]) {
        client->state = STATE_READY;
        printf("Player %s is ready\n", client->username);
    } else {
        client->state = STATE_IN_ROOM;
        printf("Player %s is not ready\n", client->username);
    }
    
    // Send status update to all
    send_room_status(server, room_id);
    
    // Check if all players are ready
    if (room->player_count == PLAYERS_PER_ROOM) {
        int all_ready = 1;
        for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
            if (room->player_ids[i] >= 0 && !room->player_ready[i]) {
                all_ready = 0;
                break;
            }
        }
        
        if (all_ready) {
            // Start the game!
            room_start_game(server, room_id);
        }
    }
}

// Handle start game request (only from host)
void handle_start_game(Server *server, int client_idx) {
    Client *client = &server->clients[client_idx];
    
    if (client->state != STATE_IN_ROOM && client->state != STATE_READY) {
        client_send(client, "ERROR|Must be in room to start game\n");
        return;
    }
    
    int room_id = client->room_id;
    Room *room = &server->rooms[room_id];
    
    // Check if client is the host
    if (client->player_index != room->host_index) {
        client_send(client, "ERROR|Only the host can start the game\n");
        return;
    }
    
    // TODO: FOR TESTING - Allow starting with less than 4 players
    // PRODUCTION: Uncomment this block to require 4 players
    /*
    if (room->player_count < PLAYERS_PER_ROOM) {
        char msg[128];
        snprintf(msg, sizeof(msg), "ERROR|Need %d players to start game (currently %d)\n", 
                 PLAYERS_PER_ROOM, room->player_count);
        client_send(client, msg);
        return;
    }
    */
    
    // For testing: Allow at least 1 player
    if (room->player_count < 1) {
        client_send(client, "ERROR|Need at least 1 player to start game\n");
        return;
    }
    
    // Start the game!
    printf("Host %s starting game in room %d with %d players\n", 
           client->username, room_id, room->player_count);
    room_start_game(server, room_id);
}

// Broadcast message to all players in room
void room_broadcast(Server *server, int room_id, const char *message, int exclude_client_idx) {
    Room *room = &server->rooms[room_id];
    
    for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
        int client_idx = room->player_ids[i];
        if (client_idx >= 0 && client_idx != exclude_client_idx) {
            client_send(&server->clients[client_idx], message);
        }
    }
}

// Clean up room
void room_cleanup(Server *server, int room_id) {
    Room *room = &server->rooms[room_id];
    
    printf("Cleaning up room %d\n", room_id);
    
    // Remove all players from room
    for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
        int client_idx = room->player_ids[i];
        if (client_idx >= 0 && server->clients[client_idx].active) {
            server->clients[client_idx].room_id = -1;
            server->clients[client_idx].player_index = -1;
            server->clients[client_idx].state = STATE_IN_LOBBY;
        }
    }
    
    room->active = 0;
}

// Send room list to client
void send_room_list(Server *server, int client_idx) {
    Client *client = &server->clients[client_idx];
    
    char buffer[BUFFER_SIZE];
    int offset = 0;
    
    offset += snprintf(buffer + offset, BUFFER_SIZE - offset, "ROOM_LIST");
    
    for (int i = 0; i < MAX_ROOMS; i++) {
        Room *room = &server->rooms[i];
        if (room->active && !room->game_started) {
            offset += snprintf(buffer + offset, BUFFER_SIZE - offset,
                             "|%d:%s:%d", room->id, room->name, room->player_count);
        }
    }
    
    offset += snprintf(buffer + offset, BUFFER_SIZE - offset, "\n");
    client_send(client, buffer);
}

// Send room status to all players in room
void send_room_status(Server *server, int room_id) {
    Room *room = &server->rooms[room_id];
    
    char buffer[BUFFER_SIZE];
    int offset = 0;
    
    offset += snprintf(buffer + offset, BUFFER_SIZE - offset, "ROOM_STATUS|%d|%d", room->player_count, room->host_index);
    
    for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
        int client_idx = room->player_ids[i];
        if (client_idx >= 0) {
            Client *c = &server->clients[client_idx];
            
            // Use stored ping value (calculated when PONG is received)
            offset += snprintf(buffer + offset, BUFFER_SIZE - offset,
                             "|%d:%s:%d:%d", i, c->username, room->player_ready[i], c->ping_ms);
        }
    }
    
    offset += snprintf(buffer + offset, BUFFER_SIZE - offset, "\n");
    room_broadcast(server, room_id, buffer, -1);
}

