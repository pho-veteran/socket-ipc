/**
 * @file server_net.h
 * @brief Server network layer functions
 * 
 * Network I/O functions for server implementation.
 */

#ifndef SERVER_NET_H
#define SERVER_NET_H

#include <stdint.h>

/**
 * @brief Setup Unix domain socket
 * @param socket_path Path to socket file
 * @return Socket file descriptor on success, -1 on error
 */
int setup_unix_socket(const char* socket_path);

/**
 * @brief Setup Internet socket
 * @param host Host address
 * @param port Port number
 * @return Socket file descriptor on success, -1 on error
 */
int setup_inet_socket(const char* host, uint16_t port);

/**
 * @brief Initialize TLS server context
 * @return SSL_CTX pointer on success, NULL on error
 */
void* init_tls_server(void);

/**
 * @brief Accept TLS connection
 * @param fd Socket file descriptor
 * @param ssl_ctx SSL context
 * @return SSL pointer on success, NULL on error
 */
void* accept_tls_connection(int fd, void* ssl_ctx);

#endif // SERVER_NET_H

