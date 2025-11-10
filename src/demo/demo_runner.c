/**
 * @file demo_runner.c
 * @brief Demo orchestrator
 * 
 * Simulates multiple clients connecting/disconnecting and sending messages.
 */

#include "demo_runner.h"
#include "../server/server.h"
#include "../client/client.h"
#include "../common/logger.h"
#include "../common/protocol.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NUM_CLIENTS 3

static Server* g_demo_server = NULL;
static MessageHandler g_demo_handler = NULL;

static void* server_thread_wrapper(void* arg) {
    Server* srv = (Server*)arg;
    server_start(srv, g_demo_handler);
    return NULL;
}

static void demo_message_handler(int client_id, const Message* msg) {
    (void)client_id;
    if (msg->header.type == MSG_TYPE_TEXT && msg->payload) {
        char* text = malloc(msg->payload_size + 1);
        if (text) {
            memcpy(text, msg->payload, msg->payload_size);
            text[msg->payload_size] = '\0';
            logger_info("Client %d sent text: %.50s", client_id, text);
            free(text);
        }
    }
}

static void* demo_client_thread(void* arg) {
    int client_id = *(int*)arg;
    free(arg);
    
    if (!g_demo_server) {
        return NULL;
    }
    
    Client client;
    if (client_init(&client, g_demo_server->mode, g_demo_server->address, 
                   g_demo_server->enable_tls) < 0) {
        logger_error("Client %d: Failed to initialize", client_id);
        return NULL;
    }
    
    if (client_connect(&client, 5) < 0) {
        logger_error("Client %d: Failed to connect", client_id);
        client_cleanup(&client);
        return NULL;
    }
    
    // Send text messages
    for (int i = 0; i < 3; i++) {
        char text[128];
        snprintf(text, sizeof(text), "Hello from client %d, message %d", client_id, i);
        
        if (client_send_text(&client, text, strlen(text)) < 0) {
            logger_error("Client %d: Failed to send message", client_id);
        }
        
        usleep(100000); // 100ms
    }
    
    client_disconnect(&client);
    client_cleanup(&client);
    logger_info("Client %d disconnected", client_id);
    
    return NULL;
}

int demo_run(SocketMode mode, const char* address, int use_tls) {
    logger_info("Starting demo...");
    
    // Initialize server
    Server server;
    if (server_init(&server, mode, address, use_tls) < 0) {
        logger_error("Failed to initialize server");
        return -1;
    }
    
    g_demo_server = &server;
    g_demo_handler = demo_message_handler;
    
    // Start server in background thread
    pthread_t server_thread;
    if (pthread_create(&server_thread, NULL, server_thread_wrapper, &server) != 0) {
        logger_error("Failed to create server thread");
        server_cleanup(&server);
        return -1;
    }
    
    // Wait for server to start
    usleep(500000); // 500ms
    
    // Create client threads
    pthread_t client_threads[NUM_CLIENTS];
    for (int i = 0; i < NUM_CLIENTS; i++) {
        int* client_id = malloc(sizeof(int));
        *client_id = i;
        
        if (pthread_create(&client_threads[i], NULL, demo_client_thread, client_id) != 0) {
            logger_error("Failed to create client thread %d", i);
            free(client_id);
        }
        
        usleep(200000); // 200ms between clients
    }
    
    // Wait for all clients to finish
    for (int i = 0; i < NUM_CLIENTS; i++) {
        pthread_join(client_threads[i], NULL);
    }
    
    // Stop server
    server_stop(&server);
    pthread_join(server_thread, NULL);
    
    server_cleanup(&server);
    logger_info("Demo completed");
    
    return 0;
}

