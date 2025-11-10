/**
 * @file utils.h
 * @brief Utility functions
 * 
 * Helper functions for socket operations and address parsing.
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>

/**
 * @brief Socket address union for AF_UNIX and AF_INET
 */
typedef union {
    struct sockaddr_un unix_addr;
    struct sockaddr_in inet_addr;
    struct sockaddr generic;
} SocketAddress;

/**
 * @brief Parse IPv4 address and port
 * @param address String in format "host:port" or just "host"
 * @param port Port number (output parameter)
 * @return Host address string (must be freed by caller)
 */
char* parse_address(const char* address, uint16_t* port);

/**
 * @brief Set socket timeout
 * @param fd Socket file descriptor
 * @param timeout_sec Timeout in seconds
 * @return 0 on success, -1 on error
 */
int set_socket_timeout(int fd, int timeout_sec);

/**
 * @brief Enable socket reuse address
 * @param fd Socket file descriptor
 * @return 0 on success, -1 on error
 */
int set_socket_reuse(int fd);

/**
 * @brief Get error string from errno
 */
const char* get_error_string(int errnum);

#endif // UTILS_H

