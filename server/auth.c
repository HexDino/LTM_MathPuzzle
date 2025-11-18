#include "server.h"

#define USERS_FILE "users.txt"

// Register new user
int register_user(const char *username, const char *password) {
    // Check if user already exists
    FILE *file = fopen(USERS_FILE, "r");
    if (file) {
        char line[256];
        char stored_user[MAX_USERNAME];
        
        while (fgets(line, sizeof(line), file)) {
            sscanf(line, "%[^:]:", stored_user);
            if (strcmp(stored_user, username) == 0) {
                fclose(file);
                return 0;  // User already exists
            }
        }
        fclose(file);
    }
    
    // Add new user
    file = fopen(USERS_FILE, "a");
    if (!file) {
        perror("fopen");
        return 0;
    }
    
    fprintf(file, "%s:%s\n", username, password);
    fclose(file);
    
    printf("New user registered: %s\n", username);
    return 1;
}

// Authenticate user
int authenticate_user(const char *username, const char *password) {
    FILE *file = fopen(USERS_FILE, "r");
    if (!file) {
        return 0;
    }
    
    char line[256];
    char stored_user[MAX_USERNAME];
    char stored_pass[MAX_PASSWORD];
    
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%[^:]:%s", stored_user, stored_pass);
        
        if (strcmp(stored_user, username) == 0 && strcmp(stored_pass, password) == 0) {
            fclose(file);
            return 1;
        }
    }
    
    fclose(file);
    return 0;
}

// Handle registration request
void handle_register(Server *server, int client_idx, const char *username, const char *password) {
    Client *client = &server->clients[client_idx];
    
    if (strlen(username) == 0 || strlen(password) == 0) {
        client_send(client, "ERROR|Username and password required\n");
        return;
    }
    
    if (register_user(username, password)) {
        client_send(client, "REGISTER_OK|Registration successful\n");
    } else {
        client_send(client, "ERROR|Username already exists\n");
    }
}

// Handle login request
void handle_login(Server *server, int client_idx, const char *username, const char *password) {
    Client *client = &server->clients[client_idx];
    
    if (strlen(username) == 0 || strlen(password) == 0) {
        client_send(client, "ERROR|Username and password required\n");
        return;
    }
    
    // Check if user is disconnected and can reconnect
    int disconnected_idx = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].active && i != client_idx &&
            strcmp(server->clients[i].username, username) == 0) {
            
            if (server->clients[i].state == STATE_DISCONNECTED) {
                time_t elapsed = time(NULL) - server->clients[i].disconnect_time;
                if (elapsed < RECONNECT_TIMEOUT) {
                    disconnected_idx = i;
                    break;
                }
            } else {
                client_send(client, "ERROR|User already logged in\n");
                return;
            }
        }
    }
    
    if (!authenticate_user(username, password)) {
        client_send(client, "ERROR|Invalid username or password\n");
        return;
    }
    
    // Handle reconnection
    if (disconnected_idx >= 0) {
        Client *old_client = &server->clients[disconnected_idx];
        
        printf("User %s reconnecting! Restoring session...\n", username);
        
        // Transfer state to new connection
        int old_room_id = old_client->room_id;
        int old_player_index = old_client->player_index;
        ClientState old_state = old_client->saved_state;
        
        // Copy username and restore state to new client
        strncpy(client->username, username, MAX_USERNAME - 1);
        client->state = old_state;
        client->room_id = old_room_id;
        client->player_index = old_player_index;
        
        // Update room's player_ids to point to new client
        if (old_room_id >= 0) {
            Room *room = &server->rooms[old_room_id];
            room->player_ids[old_player_index] = client_idx;
            
            // Notify other players of reconnection
            char msg[256];
            snprintf(msg, sizeof(msg), "PLAYER_RECONNECTED|%s\n", username);
            room_broadcast(server, old_room_id, msg, -1);
        }
        
        // Clear old client slot
        old_client->active = 0;
        old_client->state = STATE_CONNECTED;
        old_client->room_id = -1;
        
        // Send reconnect success
        char response[128];
        snprintf(response, sizeof(response), "RECONNECT_OK|%s\n", username);
        client_send(client, response);
        
        // Send room status if in room
        if (old_room_id >= 0) {
            send_room_status(server, old_room_id);
        } else {
            send_room_list(server, client_idx);
        }
        
        return;
    }
    
    // Normal login
    strncpy(client->username, username, MAX_USERNAME - 1);
    client->state = STATE_IN_LOBBY;
    
    char msg[128];
    snprintf(msg, sizeof(msg), "LOGIN_OK|%s\n", username);
    client_send(client, msg);
    
    printf("User logged in: %s\n", username);
    
    // Send room list
    send_room_list(server, client_idx);
}

