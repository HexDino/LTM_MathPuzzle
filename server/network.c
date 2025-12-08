#include "server.h"

// Handle PONG response from client
void handle_pong(Server *server, int client_idx) {
    Client *client = &server->clients[client_idx];
    time_t now = time(NULL);
    
    // Calculate RTT (Round Trip Time) in milliseconds
    int rtt = (int)((now - client->last_ping_time) * 1000);
    
    // Cap ping at reasonable value
    if (rtt > 9999) rtt = 9999;
    if (rtt < 0) rtt = 0;
    
    // Store the calculated ping
    client->ping_ms = rtt;
    client->last_pong_time = now;
}

// Send PING to all connected clients
void send_ping_to_all(Server *server) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].active) {
            Client *client = &server->clients[i];
            client->last_ping_time = time(NULL);
            client_send(client, "PING\n");
        }
    }
}

// Check for ping timeouts and disconnect dead clients
void check_ping_timeouts(Server *server) {
    time_t now = time(NULL);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].active) {
            Client *client = &server->clients[i];
            
            // Check if client hasn't responded to PONG
            if (now - client->last_pong_time > PING_TIMEOUT) {
                printf("Client %s timed out (no PONG)\n", 
                       client->username[0] ? client->username : "unknown");
                client_disconnect(server, i);
            }
        }
    }
}

// Handle chat message
void handle_chat(Server *server, int client_idx, const char *message) {
    Client *client = &server->clients[client_idx];
    
    if (client->room_id < 0) {
        client_send(client, "ERROR|Must be in a room to chat\n");
        return;
    }
    
    // Broadcast chat to all players in room
    char msg[BUFFER_SIZE];
    snprintf(msg, sizeof(msg), "CHAT|%s|%s\n", client->username, message);
    room_broadcast(server, client->room_id, msg, -1);
    
    printf("Chat from %s in room %d: %s\n", client->username, client->room_id, message);
}

// Shutdown server gracefully
void server_shutdown(Server *server) {
    printf("Shutting down server...\n");
    
    // Disconnect all clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].active) {
            client_send(&server->clients[i], "SERVER_SHUTDOWN|Server is shutting down\n");
            client_disconnect(server, i);
        }
    }
    
    // Close listening socket
    close(server->listen_fd);
    
    printf("Server shutdown complete\n");
}

