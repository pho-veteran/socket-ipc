/**
 * @file server/main.c
 * @brief Server entry point
 * 
 * Reads configuration from server_input.txt and writes logs to server_output.txt
 */

#define _POSIX_C_SOURCE 200809L
#include "server.h"
#include "../common/logger.h"
#include "../common/types.h"
#include "../common/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define MAX_LINE_LEN 256
#define INPUT_FILE "server_input.txt"
#define OUTPUT_FILE "server_output.txt"

static Server g_server;
static int g_running = 1;

static void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
    server_stop(&g_server);
    logger_info("Received signal, shutting down...");
}

static int parse_config(const char* filename, SocketMode* mode, char** address, int* enable_tls) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return -1;
    }
    
    char line[MAX_LINE_LEN];
    *mode = SOCKET_MODE_INET;
    *address = NULL;
    *enable_tls = 0;
    
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

static void message_handler(int client_id, const Message* msg) {
    (void)client_id;
    if (msg->header.type == MSG_TYPE_TEXT && msg->payload) {
        char* text = malloc(msg->payload_size + 1);
        if (text) {
            memcpy(text, msg->payload, msg->payload_size);
            text[msg->payload_size] = '\0';
            logger_info("Received text message: %s", text);
            free(text);
        }
    }
}

static void write_metrics(FILE* f, const Server* server) {
    size_t total_clients, total_messages;
    double uptime, throughput_mb_s, avg_latency_ms, min_latency_ms, max_latency_ms;
    
    server_get_metrics(server, &total_clients, &total_messages, &uptime,
                      &throughput_mb_s, &avg_latency_ms, &min_latency_ms, &max_latency_ms);
    
    fprintf(f, "\n=== SERVER METRICS ===\n");
    fprintf(f, "Mode: %s\n", server->mode == SOCKET_MODE_UNIX ? "unix" : "inet");
    fprintf(f, "Address: %s\n", server->address);
    if (server->mode == SOCKET_MODE_INET) {
        fprintf(f, "TLS Enabled: %s\n", server->enable_tls ? "Yes" : "No");
    }
    fprintf(f, "Total Clients: %zu\n", total_clients);
    fprintf(f, "Total Messages Received: %zu\n", total_messages);
    fprintf(f, "Uptime: %.2f seconds\n", uptime);
    
    double message_rate = (uptime > 0.0) ? (double)total_messages / uptime : 0.0;
    fprintf(f, "Message Rate: %.2f msg/s\n", message_rate);
    fprintf(f, "Average Latency: %.2f ms\n", avg_latency_ms);
    fprintf(f, "Throughput: %.4f MB/s\n", throughput_mb_s);
    fflush(f);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Parse configuration
    SocketMode mode;
    char* address = NULL;
    int enable_tls = 0;
    
    if (parse_config(INPUT_FILE, &mode, &address, &enable_tls) < 0) {
        return 1;
    }
    
    // Initialize logger with output file
    if (logger_init(OUTPUT_FILE) < 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        free(address);
        return 1;
    }
    
    if (mode == SOCKET_MODE_UNIX && enable_tls) {
        logger_warn("TLS requested for UNIX mode; disabling TLS because it is not required.");
        enable_tls = 0;
    }
    
    if (mode == SOCKET_MODE_UNIX) {
        logger_info("Server configuration: mode=unix, path=%s, tls=%s",
                    address ? address : "(null)",
                    enable_tls ? "enabled" : "disabled");
    } else {
        logger_info("Server configuration: mode=inet, address=%s, tls=%s",
                    address ? address : "(null)",
                    enable_tls ? "enabled" : "disabled");
    }
    
    // Initialize server
    if (server_init(&g_server, mode, address, enable_tls) < 0) {
        logger_error("Failed to initialize server");
        free(address);
        logger_cleanup();
        return 1;
    }
    
    free(address);
    
    // Start server
    logger_info("Starting server...");
    if (server_start(&g_server, message_handler) < 0) {
        logger_error("Failed to start server");
        server_cleanup(&g_server);
        logger_cleanup();
        return 1;
    }
    
    // Write metrics to output file
    FILE* output = fopen(OUTPUT_FILE, "a");
    if (output) {
        write_metrics(output, &g_server);
        fclose(output);
    }
    
    // Cleanup
    server_cleanup(&g_server);
    logger_cleanup();
    
    return 0;
}

