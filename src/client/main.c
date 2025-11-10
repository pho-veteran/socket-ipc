/**
 * @file client/main.c
 * @brief Client entry point
 * 
 * Reads configuration and messages from client_input.txt and writes status to client_output.txt
 */

#include "client.h"
#include "../common/logger.h"
#include "../common/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LEN 1024
#define INPUT_FILE "client_input.txt"
#define OUTPUT_FILE "client_output.txt"

static int parse_config(const char* filename, SocketMode* mode, char** address, 
                       int* enable_tls, int* free_input, char*** messages, size_t* message_count) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return -1;
    }
    
    char line[MAX_LINE_LEN];
    *mode = SOCKET_MODE_INET;
    *address = NULL;
    *enable_tls = 0;
    *free_input = 0;
    *messages = NULL;
    *message_count = 0;
    size_t message_capacity = 0;
    
    while (fgets(line, sizeof(line), f)) {
        // Remove newline
        line[strcspn(line, "\n")] = '\0';
        
        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        
        char* eq = strchr(line, '=');
        if (!eq) {
            continue;
        }
        
        *eq = '\0';
        char* key = line;
        char* value = eq + 1;
        
        if (strcmp(key, "mode") == 0) {
            if (strcmp(value, "unix") == 0) {
                *mode = SOCKET_MODE_UNIX;
            } else if (strcmp(value, "inet") == 0) {
                *mode = SOCKET_MODE_INET;
            }
        } else if (strcmp(key, "address") == 0) {
            *address = strdup(value);
        } else if (strcmp(key, "tls") == 0) {
            *enable_tls = (atoi(value) != 0);
        } else if (strcmp(key, "free_input") == 0) {
            *free_input = (atoi(value) != 0);
        } else if (strcmp(key, "message") == 0) {
            // Add message to list
            if (*message_count >= message_capacity) {
                size_t new_capacity = message_capacity == 0 ? 8 : message_capacity * 2;
                char** new_messages = realloc(*messages, new_capacity * sizeof(char*));
                if (!new_messages) {
                    // Cleanup on error
                    for (size_t i = 0; i < *message_count; i++) {
                        free((*messages)[i]);
                    }
                    free(*messages);
                    free(*address);
                    fclose(f);
                    return -1;
                }
                *messages = new_messages;
                message_capacity = new_capacity;
            }
            (*messages)[*message_count] = strdup(value);
            (*message_count)++;
        }
    }
    
    fclose(f);
    
    // Set default address if not provided
    if (!*address) {
        if (*mode == SOCKET_MODE_UNIX) {
            *address = strdup("/tmp/server.sock");
        } else {
            *address = strdup("localhost:8080");
        }
    }
    
    return 0;
}

static void free_messages(char** messages, size_t count) {
    if (messages) {
        for (size_t i = 0; i < count; i++) {
            free(messages[i]);
        }
        free(messages);
    }
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    // Parse configuration
    SocketMode mode;
    char* address = NULL;
    int enable_tls = 0;
    int free_input = 0;
    char** messages = NULL;
    size_t message_count = 0;
    
    if (parse_config(INPUT_FILE, &mode, &address, &enable_tls, &free_input, &messages, &message_count) < 0) {
        return 1;
    }
    
    // Initialize logger with output file
    if (logger_init(OUTPUT_FILE) < 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        free(address);
        free_messages(messages, message_count);
        return 1;
    }
    
    // Initialize client
    Client client;
    
    if (mode == SOCKET_MODE_UNIX && enable_tls) {
        logger_warn("TLS requested for UNIX mode; disabling TLS because it is not required.");
        enable_tls = 0;
    }
    
    if (client_init(&client, mode, address, enable_tls) < 0) {
        logger_error("Failed to initialize client");
        free(address);
        free_messages(messages, message_count);
        logger_cleanup();
        return 1;
    }
    
    free(address);
    
    // Connect to server
    logger_info("Connecting to server...");
    if (client_connect(&client, 5) < 0) {
        logger_error("Failed to connect to server");
        client_cleanup(&client);
        free_messages(messages, message_count);
        logger_cleanup();
        return 1;
    }
    
    logger_info("Connected successfully");
    
    if (free_input) {
        // Interactive mode: keep connection open and read from stdin
        printf("Connected. Type messages to send (type 'quit' or 'exit' to close):\n");
        char line[MAX_LINE_LEN];
        while (fgets(line, sizeof(line), stdin)) {
            // Remove newline
            line[strcspn(line, "\n")] = '\0';
            if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
                break;
            }
            if (line[0] == '\0') {
                continue;
            }
            logger_info("Sending message: %s", line);
            if (client_send_text(&client, line, strlen(line)) < 0) {
                logger_error("Failed to send message: %s", line);
                // Continue allowing user to try again
            } else {
                logger_info("Message sent successfully: %s", line);
            }
        }
    } else {
        // Send messages from config
        for (size_t i = 0; i < message_count; i++) {
            logger_info("Sending message: %s", messages[i]);
            if (client_send_text(&client, messages[i], strlen(messages[i])) < 0) {
                logger_error("Failed to send message: %s", messages[i]);
            } else {
                logger_info("Message sent successfully: %s", messages[i]);
            }
        }
    }
    
    // Disconnect
    client_disconnect(&client);
    client_cleanup(&client);
    
    // Free messages
    free_messages(messages, message_count);
    
    logger_cleanup();
    
    return 0;
}

