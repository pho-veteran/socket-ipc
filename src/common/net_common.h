/**
 * @file net_common.h
 * @brief Common network utilities header
 */

#ifndef NET_COMMON_H
#define NET_COMMON_H

#include "protocol.h"

/**
 * @brief Send message (with TLS support)
 * @param fd Socket file descriptor
 * @param ssl SSL context (NULL if not using TLS)
 * @param is_ssl Whether TLS is enabled
 * @param msg Message to send
 * @return 0 on success, -1 on error
 */
int send_message(int fd, void* ssl, int is_ssl, const Message* msg);

/**
 * @brief Receive message (with TLS support)
 * @param fd Socket file descriptor
 * @param ssl SSL context (NULL if not using TLS)
 * @param is_ssl Whether TLS is enabled
 * @param msg Output message (must be freed with message_free)
 * @return 0 on success, -1 on error
 */
int receive_message(int fd, void* ssl, int is_ssl, Message* msg);

/**
 * @brief Close TLS connection
 */
void close_tls_connection(void* ssl);

/**
 * @brief Cleanup TLS context
 */
void cleanup_tls(void* ssl_ctx);

#endif // NET_COMMON_H

