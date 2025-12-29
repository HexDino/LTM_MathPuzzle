#include "server.h"

// Get operator string
char* get_operator_string(Operator op) {
    switch (op) {
        case OP_ADD: return "+";
        case OP_SUB: return "-";
        case OP_MUL: return "*";
        case OP_DIV: return "/";
        default: return "?";
    }
}

// Apply operator
int apply_operator(int a, Operator op, int b) {
    switch (op) {
        case OP_ADD: return a + b;
        case OP_SUB: return a - b;
        case OP_MUL: return a * b;
        case OP_DIV: return (b != 0) ? a / b : a;
        default: return a;
    }
}

// Calculate result based on operators
// Format: P1 op1 P2 op2 P3 = result
// Example: P1 - P2 * P3 = result (with proper precedence)
int calculate_result(int p1, Operator op1, int p2, Operator op2, int p3) {
    // Handle operator precedence
    // If op2 is multiplication, do it first
    if (op2 == OP_MUL) {
        int temp = p2 * p3;
        if (op1 == OP_ADD) return p1 + temp;
        else if (op1 == OP_SUB) return p1 - temp;
        else return p1 * temp;
    }
    
    // Otherwise, left to right
    int temp;
    if (op1 == OP_ADD) temp = p1 + p2;
    else if (op1 == OP_SUB) temp = p1 - p2;
    else temp = p1 * p2;
    
    if (op2 == OP_ADD) return temp + p3;
    else if (op2 == OP_SUB) return temp - p3;
    else return temp * p3;
}

// Generate random puzzle based on round difficulty
void puzzle_generate(Puzzle *puzzle, int round) {
    puzzle->round = round;
    int p1, p2, p3, p4;
    int min_val, max_val;
    int allow_negative = 0;
    
    // Configure difficulty based on round
    switch (round) {
        case 1: // Easy: Addition/Subtraction, format P1 ± P2 ± P3 = P4
            puzzle->format = FORMAT_P1_P2_P3_EQ_P4;
            puzzle->op1 = rand() % 2;  // Only ADD or SUB
            puzzle->op2 = rand() % 2;
            min_val = 1; max_val = 50;
            break;
            
        case 2: // Medium: Add/Sub with larger numbers, format P1 ± P2 = P3 ± P4
            puzzle->format = FORMAT_P1_P2_EQ_P3_P4;
            puzzle->op1 = rand() % 2;
            puzzle->op2 = rand() % 2;
            min_val = 10; max_val = 80;
            break;
            
        case 3: // Hard: Include multiplication, format P1 = P2 * P3 ± P4
            puzzle->format = FORMAT_P1_EQ_P2_P3_P4;
            puzzle->op1 = OP_MUL;
            puzzle->op2 = rand() % 2;  // ADD or SUB
            min_val = 2; max_val = 30;
            break;
            
        case 4: // Very Hard: Mixed operations, format P1 * P2 = P3 ± P4
            puzzle->format = FORMAT_P1_P2_EQ_P3_P4;
            puzzle->op1 = OP_MUL;
            puzzle->op2 = rand() % 3;  // ADD, SUB, or MUL
            min_val = 2; max_val = 40;
            break;
            
        case 5: // Expert: All operations + negative numbers, format P1 * P2 ± P3 = P4
            puzzle->format = FORMAT_P1_P2_P3_EQ_P4;
            puzzle->op1 = (rand() % 2) ? OP_MUL : OP_DIV;
            puzzle->op2 = rand() % 3;
            min_val = -20; max_val = 50;
            allow_negative = 1;
            break;
            
        default:
            puzzle->format = FORMAT_P1_P2_P3_EQ_P4;
            puzzle->op1 = rand() % 2;
            puzzle->op2 = rand() % 2;
            min_val = 1; max_val = 50;
    }
    
    // Generate values based on equation format
    switch (puzzle->format) {
        case FORMAT_P1_P2_P3_EQ_P4: // P1 op1 P2 op2 P3 = P4
            p1 = (rand() % (max_val - min_val + 1)) + min_val;
            p2 = (rand() % (max_val - min_val + 1)) + min_val;
            p3 = (rand() % (max_val - min_val + 1)) + min_val;
            // Use calculate_result to handle operator precedence correctly
            p4 = calculate_result(p1, puzzle->op1, p2, puzzle->op2, p3);
            break;
            
        case FORMAT_P1_EQ_P2_P3_P4: // P1 = P2 op1 P3 op2 P4
            p2 = (rand() % (max_val - min_val + 1)) + min_val;
            p3 = (rand() % (max_val - min_val + 1)) + min_val;
            p4 = (rand() % (max_val - min_val + 1)) + min_val;
            // Use calculate_result to handle operator precedence correctly
            p1 = calculate_result(p2, puzzle->op1, p3, puzzle->op2, p4);
            break;
            
        case FORMAT_P1_P2_EQ_P3_P4: // P1 op1 P2 = P3 op2 P4
            p3 = (rand() % (max_val - min_val + 1)) + min_val;
            p4 = (rand() % (max_val - min_val + 1)) + min_val;
            p1 = (rand() % (max_val - min_val + 1)) + min_val;
            
            // Calculate right side: P3 op2 P4
            int right_side = apply_operator(p3, puzzle->op2, p4);
            
            // Calculate P2 based on left side operator: P1 op1 P2 = right_side
            if (puzzle->op1 == OP_ADD) {
                p2 = right_side - p1;  // P1 + P2 = right_side => P2 = right_side - P1
            } else if (puzzle->op1 == OP_SUB) {
                p2 = p1 - right_side;  // P1 - P2 = right_side => P2 = P1 - right_side
            } else if (puzzle->op1 == OP_MUL) {
                p2 = (p1 != 0) ? right_side / p1 : right_side;  // P1 * P2 = right_side => P2 = right_side / P1
            } else { // OP_DIV
                p2 = (right_side != 0) ? p1 / right_side : p1;  // P1 / P2 = right_side => P2 = P1 / right_side
            }
            break;
    }
    
    // Store solution values
    puzzle->solution_values[0] = p1;
    puzzle->solution_values[1] = p2;
    puzzle->solution_values[2] = p3;
    puzzle->solution_values[3] = p4;
    puzzle->result = (puzzle->format == FORMAT_P1_EQ_P2_P3_P4) ? p1 : p4;
    
    // Generate matrices with random numbers
    for (int m = 0; m < PLAYERS_PER_ROOM; m++) {
        for (int i = 0; i < MATRIX_SIZE; i++) {
            for (int j = 0; j < MATRIX_SIZE; j++) {
                if (allow_negative) {
                    puzzle->matrices[m].data[i][j] = (rand() % (max_val - min_val + 1)) + min_val;
                } else {
                    puzzle->matrices[m].data[i][j] = (rand() % max_val) + 1;
                }
            }
        }
        
        // Place solution value at random position
        puzzle->solution_row[m] = rand() % MATRIX_SIZE;
        puzzle->solution_col[m] = rand() % MATRIX_SIZE;
        puzzle->matrices[m].data[puzzle->solution_row[m]][puzzle->solution_col[m]] = puzzle->solution_values[m];
    }
    
    // Print puzzle based on actual format
    printf("Round %d puzzle generated (format %d): ", round, puzzle->format);
    switch (puzzle->format) {
        case FORMAT_P1_P2_P3_EQ_P4:
            printf("P1[%d] %s P2[%d] %s P3[%d] = P4[%d]\n",
                   puzzle->solution_values[0], get_operator_string(puzzle->op1),
                   puzzle->solution_values[1], get_operator_string(puzzle->op2),
                   puzzle->solution_values[2], puzzle->solution_values[3]);
            break;
        case FORMAT_P1_EQ_P2_P3_P4:
            printf("P1[%d] = P2[%d] %s P3[%d] %s P4[%d]\n",
                   puzzle->solution_values[0], puzzle->solution_values[1],
                   get_operator_string(puzzle->op1), puzzle->solution_values[2],
                   get_operator_string(puzzle->op2), puzzle->solution_values[3]);
            break;
        case FORMAT_P1_P2_EQ_P3_P4:
            printf("P1[%d] %s P2[%d] = P3[%d] %s P4[%d]\n",
                   puzzle->solution_values[0], get_operator_string(puzzle->op1),
                   puzzle->solution_values[1], puzzle->solution_values[2],
                   get_operator_string(puzzle->op2), puzzle->solution_values[3]);
            break;
    }
}

// Send puzzle to clients (asymmetric information)
void puzzle_send_to_clients(Server *server, int room_id) {
    Room *room = &server->rooms[room_id];
    Puzzle *puzzle = &room->puzzle;
    
    // Send to each player, hiding their own matrix
    for (int player = 0; player < PLAYERS_PER_ROOM; player++) {
        int client_idx = room->player_ids[player];
        if (client_idx < 0) continue;
        
        Client *client = &server->clients[client_idx];
        
        char buffer[BUFFER_SIZE * 2];
        int offset = 0;
        
        // Format: GAME_START|equation|matrix0|matrix1|matrix2|matrix3
        // matrix format: 16 numbers separated by commas (or HIDDEN)
        
        // Build equation string based on format
        char equation[128];
        switch (puzzle->format) {
            case FORMAT_P1_P2_P3_EQ_P4:
                snprintf(equation, sizeof(equation), "P1%sP2%sP3=P4",
                        get_operator_string(puzzle->op1),
                        get_operator_string(puzzle->op2));
                break;
            case FORMAT_P1_EQ_P2_P3_P4:
                snprintf(equation, sizeof(equation), "P1=P2%sP3%sP4",
                        get_operator_string(puzzle->op1),
                        get_operator_string(puzzle->op2));
                break;
            case FORMAT_P1_P2_EQ_P3_P4:
                snprintf(equation, sizeof(equation), "P1%sP2=P3%sP4",
                        get_operator_string(puzzle->op1),
                        get_operator_string(puzzle->op2));
                break;
            default:
                snprintf(equation, sizeof(equation), "P1%sP2%sP3=P4",
                        get_operator_string(puzzle->op1),
                        get_operator_string(puzzle->op2));
        }
        
        offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                         "GAME_START|%s", equation);
        
        // Send all matrices except the player's own
        for (int m = 0; m < PLAYERS_PER_ROOM; m++) {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "|");
            
            if (m == player) {
                // Hide this player's matrix
                offset += snprintf(buffer + offset, sizeof(buffer) - offset, "HIDDEN");
            } else {
                // Send matrix data
                for (int i = 0; i < MATRIX_SIZE; i++) {
                    for (int j = 0; j < MATRIX_SIZE; j++) {
                        if (i == 0 && j == 0) {
                            offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                                             "%d", puzzle->matrices[m].data[i][j]);
                        } else {
                            offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                                             ",%d", puzzle->matrices[m].data[i][j]);
                        }
                    }
                }
            }
        }
        
        // Add round info at the end
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "|%d|%d\n",
                         room->current_round, room->total_rounds);
        
        client_send(client, buffer);
        printf("Sent puzzle to player %d (hiding matrix %d)\n", player, player);
    }
}

// Start game in room
void room_start_game(Server *server, int room_id) {
    Room *room = &server->rooms[room_id];
    
    // Initialize round system on first start
    if (!room->game_started) {
        room->current_round = 1;
        room->total_rounds = 5;
    }
    
    printf("Starting game in room %d, round %d/%d\n", room_id, room->current_round, room->total_rounds);
    
    // Generate puzzle for current round
    puzzle_generate(&room->puzzle, room->current_round);
    
    // Initialize game state
    room->game_started = 1;
    room->game_start_time = time(NULL);
    room->game_time_remaining = GAME_DURATION;
    room->all_submitted = 0;
    room->waiting_for_continue = 0;  // Reset waiting state when starting new round
    
    for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
        room->answer_submitted[i] = 0;
        room->round_continue_ready[i] = 0;  // Reset continue ready flags
        int client_idx = room->player_ids[i];
        if (client_idx >= 0) {
            server->clients[client_idx].state = STATE_IN_GAME;
        }
    }
    
    // Send puzzle to all players
    puzzle_send_to_clients(server, room_id);
}

// End game in room
void room_end_game(Server *server, int room_id, int won, int timeout) {
    Room *room = &server->rooms[room_id];
    
    printf("Ending round %d in room %d, result: %s\n", room->current_round, room_id, won ? "WIN" : "LOSE");
    
    char msg[512];
    if (won) {
        // Check if there are more rounds
        if (room->current_round < room->total_rounds) {
            // Round completed - wait for all players to continue
            snprintf(msg, sizeof(msg), "ROUND_END|WIN|Round %d/%d complete! Waiting for all players to continue...\n",
                    room->current_round, room->total_rounds);
            room_broadcast(server, room_id, msg, -1);
            
            // Set waiting state and reset continue ready flags
            room->waiting_for_continue = 1;
            for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
                room->round_continue_ready[i] = 0;
            }
            
            printf("Waiting for all players to continue to round %d\n", room->current_round + 1);
            return;  // Don't start next round yet, wait for READY_NEXT_ROUND from all players
        } else {
            // All rounds completed - player wins!
            snprintf(msg, sizeof(msg), "GAME_END|WIN|Congratulations! You completed all %d rounds!\n",
                    room->total_rounds);
        }
    } else {
        // Show the correct solution with reason based on equation format
        const char *reason = timeout ? "Time's up!" : "Wrong answer!";
        char solution[256];
        
        switch (room->puzzle.format) {
            case FORMAT_P1_P2_P3_EQ_P4:
                snprintf(solution, sizeof(solution),
                        "P1[%d,%d]=%d %s P2[%d,%d]=%d %s P3[%d,%d]=%d = P4[%d,%d]=%d",
                        room->puzzle.solution_row[0], room->puzzle.solution_col[0], room->puzzle.solution_values[0],
                        get_operator_string(room->puzzle.op1),
                        room->puzzle.solution_row[1], room->puzzle.solution_col[1], room->puzzle.solution_values[1],
                        get_operator_string(room->puzzle.op2),
                        room->puzzle.solution_row[2], room->puzzle.solution_col[2], room->puzzle.solution_values[2],
                        room->puzzle.solution_row[3], room->puzzle.solution_col[3], room->puzzle.solution_values[3]);
                break;
                
            case FORMAT_P1_EQ_P2_P3_P4:
                snprintf(solution, sizeof(solution),
                        "P1[%d,%d]=%d = P2[%d,%d]=%d %s P3[%d,%d]=%d %s P4[%d,%d]=%d",
                        room->puzzle.solution_row[0], room->puzzle.solution_col[0], room->puzzle.solution_values[0],
                        room->puzzle.solution_row[1], room->puzzle.solution_col[1], room->puzzle.solution_values[1],
                        get_operator_string(room->puzzle.op1),
                        room->puzzle.solution_row[2], room->puzzle.solution_col[2], room->puzzle.solution_values[2],
                        get_operator_string(room->puzzle.op2),
                        room->puzzle.solution_row[3], room->puzzle.solution_col[3], room->puzzle.solution_values[3]);
                break;
                
            case FORMAT_P1_P2_EQ_P3_P4:
                snprintf(solution, sizeof(solution),
                        "P1[%d,%d]=%d %s P2[%d,%d]=%d = P3[%d,%d]=%d %s P4[%d,%d]=%d",
                        room->puzzle.solution_row[0], room->puzzle.solution_col[0], room->puzzle.solution_values[0],
                        get_operator_string(room->puzzle.op1),
                        room->puzzle.solution_row[1], room->puzzle.solution_col[1], room->puzzle.solution_values[1],
                        room->puzzle.solution_row[2], room->puzzle.solution_col[2], room->puzzle.solution_values[2],
                        get_operator_string(room->puzzle.op2),
                        room->puzzle.solution_row[3], room->puzzle.solution_col[3], room->puzzle.solution_values[3]);
                break;
        }
        
        snprintf(msg, sizeof(msg), "GAME_END|LOSE|%s|%s\n", reason, solution);
    }
    
    room_broadcast(server, room_id, msg, -1);
    
    // Reset room state
    room->game_started = 0;
    room->current_round = 0;
    for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
        room->player_ready[i] = 0;
        int client_idx = room->player_ids[i];
        if (client_idx >= 0) {
            server->clients[client_idx].state = STATE_IN_ROOM;
        }
    }
    
    // Send updated room status
    send_room_status(server, room_id);
}

// Verify solution
int puzzle_verify_solution(Puzzle *puzzle, int submitted[PLAYERS_PER_ROOM][2]) {
    // Check if submitted coordinates match solution
    for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
        int row = submitted[i][0];
        int col = submitted[i][1];
        
        if (row != puzzle->solution_row[i] || col != puzzle->solution_col[i]) {
            return 0;  // Wrong answer
        }
    }
    
    return 1;  // Correct!
}

// Handle submit answer
void handle_submit(Server *server, int client_idx, int row, int col) {
    Client *client = &server->clients[client_idx];
    
    if (client->state != STATE_IN_GAME) {
        client_send(client, "ERROR|Not in game\n");
        return;
    }
    
    int room_id = client->room_id;
    Room *room = &server->rooms[room_id];
    int player_idx = client->player_index;
    
    // Validate coordinates
    if (row < 0 || row >= MATRIX_SIZE || col < 0 || col >= MATRIX_SIZE) {
        client_send(client, "ERROR|Invalid coordinates\n");
        return;
    }
    
    // Store answer
    room->submitted_answers[player_idx][0] = row;
    room->submitted_answers[player_idx][1] = col;
    room->answer_submitted[player_idx] = 1;
    
    printf("Player %s submitted answer: [%d,%d]\n", client->username, row, col);
    
    // Notify all players
    char msg[128];
    snprintf(msg, sizeof(msg), "PLAYER_SUBMITTED|%d|%s\n", player_idx, client->username);
    room_broadcast(server, room_id, msg, -1);
    
    // Check if all players have submitted (and round hasn't ended yet)
    int all_submitted = 1;
    for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
        if (room->player_ids[i] >= 0 && !room->answer_submitted[i]) {
            all_submitted = 0;
            break;
        }
    }
    
    // Only end round once - check if we're already waiting for continue
    if (all_submitted && !room->waiting_for_continue && room->game_started) {
        // Verify solution
        int correct = puzzle_verify_solution(&room->puzzle, room->submitted_answers);
        room_end_game(server, room_id, correct, 0);  // 0 = not timeout
    }
}

// Broadcast timer update
void broadcast_timer_update(Server *server, int room_id) {
    Room *room = &server->rooms[room_id];
    
    char msg[64];
    snprintf(msg, sizeof(msg), "TIMER|%d\n", room->game_time_remaining);
    room_broadcast(server, room_id, msg, -1);
}

// Handle player ready for next round
void handle_ready_next_round(Server *server, int client_idx) {
    Client *client = &server->clients[client_idx];
    
    if (client->state != STATE_IN_GAME && client->state != STATE_IN_ROOM) {
        client_send(client, "ERROR|Not in a game\n");
        return;
    }
    
    int room_id = client->room_id;
    if (room_id < 0 || room_id >= MAX_ROOMS) {
        client_send(client, "ERROR|Invalid room\n");
        return;
    }
    
    Room *room = &server->rooms[room_id];
    if (!room->active || !room->waiting_for_continue) {
        client_send(client, "ERROR|Not waiting for continue\n");
        return;
    }
    
    int player_index = client->player_index;
    
    // Mark player as ready for next round
    if (room->round_continue_ready[player_index]) {
        printf("Player %s already ready for next round\n", client->username);
        return;
    }
    
    room->round_continue_ready[player_index] = 1;
    printf("Player %s ready for next round (%d/%d)\n", 
           client->username, player_index + 1, room->player_count);
    
    // Check if all players are ready
    int all_ready = 1;
    for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
        if (room->player_ids[i] >= 0 && !room->round_continue_ready[i]) {
            all_ready = 0;
            break;
        }
    }
    
    if (all_ready) {
        printf("All players ready! Starting round %d\n", room->current_round + 1);
        
        // Reset waiting state
        room->waiting_for_continue = 0;
        
        // Increment round and start next round
        room->current_round++;
        room_start_game(server, room_id);
    } else {
        // Notify this player that they're ready
        char msg[128];
        snprintf(msg, sizeof(msg), "WAIT_CONTINUE|Waiting for other players...\n");
        client_send(client, msg);
    }
}

