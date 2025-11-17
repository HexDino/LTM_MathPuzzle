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
    
    // Check if already logged in
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].active && i != client_idx &&
            strcmp(server->clients[i].username, username) == 0) {
            client_send(client, "ERROR|User already logged in\n");
            return;
        }
    }
    
    if (authenticate_user(username, password)) {
        strncpy(client->username, username, MAX_USERNAME - 1);
        client->state = STATE_IN_LOBBY;
        
        char msg[128];
        snprintf(msg, sizeof(msg), "LOGIN_OK|%s\n", username);
        client_send(client, msg);
        
        printf("User logged in: %s\n", username);
        
        // Send room list
        send_room_list(server, client_idx);
    } else {
        client_send(client, "ERROR|Invalid username or password\n");
    }
}

