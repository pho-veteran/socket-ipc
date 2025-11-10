/**
 * @file client.h
 * @brief Client implementation
 * 
 * Client with reconnect logic and timeout handling.
 */

#ifndef CLIENT_H
#define CLIENT_H

#include "../common/protocol.h"
#include "../common/types.h"
#include <stdint.h>

/**
 * @brief Client structure
 */
typedef struct {
    SocketMode mode;
    char* address;
    int enable_tls;
    int fd;
    void* ssl;  // SSL* pointer
    int connected;
    int timeout_sec;
    void* ssl_ctx;  // SSL_CTX* pointer
} Client;

/**
 * @brief Initialize client
 * @param client Client structure to initialize
 * @param mode Socket mode (UNIX or INET)
 * @param address Server address (socket path for UNIX, host:port for INET)
 * @param enable_tls Enable TLS/SSL encryption
 * @return 0 on success, -1 on error
 */
int client_init(Client* client, SocketMode mode, const char* address, int enable_tls);

/**
 * @brief Cleanup client resources
 */
void client_cleanup(Client* client);

/**
 * @brief Connect to server
 * @param client Client instance
 * @param timeout_sec Connection timeout in seconds
 * @return 0 on success, -1 on error
 */
int client_connect(Client* client, int timeout_sec);

/**
 * @brief Disconnect from server
 */
void client_disconnect(Client* client);

/**
 * @brief Check if connected
 */
int client_is_connected(const Client* client);

/**
 * @brief Send text message
 * @param client Client instance
 * @param text Text to send
 * @param text_len Length of text
 * @return 0 on success, -1 on error
 */
int client_send_text(Client* client, const char* text, size_t text_len);

/**
 * @brief Receive message
 * @param client Client instance
 * @param msg Output message (must be freed with message_free)
 * @return 0 on success, -1 on error
 */
int client_receive_message(Client* client, Message* msg);

/**
 * @brief Set receive timeout
 * @param client Client instance
 * @param timeout_sec Timeout in seconds
 */
void client_set_timeout(Client* client, int timeout_sec);

#endif // CLIENT_H

