/**
 * @file client_net.h
 * @brief Client network layer functions
 * 
 * Network I/O functions for client implementation.
 */

#ifndef CLIENT_NET_H
#define CLIENT_NET_H

#include <stdint.h>

/**
 * @brief Connect to Unix domain socket
 * @param socket_path Path to socket file
 * @return Socket file descriptor on success, -1 on error
 */
int connect_unix_socket(const char* socket_path);

/**
 * @brief Connect to Internet socket
 * @param host Host address
 * @param port Port number
 * @return Socket file descriptor on success, -1 on error
 */
int connect_inet_socket(const char* host, uint16_t port);

/**
 * @brief Initialize TLS client context
 * @return SSL_CTX pointer on success, NULL on error
 */
void* init_tls_client(void);

/**
 * @brief Connect with TLS
 * @param fd Socket file descriptor
 * @param ssl_ctx SSL context
 * @return SSL pointer on success, NULL on error
 */
void* connect_tls(int fd, void* ssl_ctx);

#endif // CLIENT_NET_H

