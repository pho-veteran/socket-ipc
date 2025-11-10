/**
 * @file server.h
 * @brief Server implementation
 * 
 * Server with multi-client support using poll() for I/O multiplexing.
 * Supports both AF_UNIX and AF_INET sockets with optional TLS.
 */

#ifndef SERVER_H
#define SERVER_H

#include "../common/protocol.h"
#include "../common/types.h"
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Client connection structure
 */
typedef struct {
    int fd;
    void* ssl;  // SSL* pointer
    int is_ssl;
} ClientConnection;

/**
 * @brief Server structure
 */
typedef struct {
    SocketMode mode;
    char* address;
    int enable_tls;
    int server_fd;
    int running;
    void* ssl_ctx;  // SSL_CTX* pointer
    
    ClientConnection* clients;
    size_t client_count;
    size_t client_capacity;
    
    // Metrics
    size_t total_clients;
    size_t total_messages;
    double start_time;
    size_t total_bytes;
    double last_message_time;
    double total_interval_ms;
    double min_interval_ms;
    double max_interval_ms;
    size_t interval_count;
} Server;

/**
 * @brief Message handler callback type
 */
typedef void (*MessageHandler)(int client_id, const Message* msg);

/**
 * @brief Initialize server
 * @param server Server structure to initialize
 * @param mode Socket mode (UNIX or INET)
 * @param address Address (socket path for UNIX, host:port for INET)
 * @param enable_tls Enable TLS/SSL encryption
 * @return 0 on success, -1 on error
 */
int server_init(Server* server, SocketMode mode, const char* address, int enable_tls);

/**
 * @brief Cleanup server resources
 */
void server_cleanup(Server* server);

/**
 * @brief Start the server
 * @param server Server instance
 * @param handler Message handler callback (can be NULL)
 * @return 0 on success, -1 on error
 */
int server_start(Server* server, MessageHandler handler);

/**
 * @brief Stop the server gracefully
 */
void server_stop(Server* server);

/**
 * @brief Get number of connected clients
 */
size_t server_get_client_count(const Server* server);

/**
 * @brief Get server metrics
 */
void server_get_metrics(const Server* server, 
                       size_t* total_clients,
                       size_t* total_messages,
                       double* uptime,
                       double* throughput_mbps,
                       double* avg_latency_ms,
                       double* min_latency_ms,
                       double* max_latency_ms);

#endif // SERVER_H

