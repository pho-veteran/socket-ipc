/**
 * @file utils.c
 * @brief Utility functions implementation
 */

#define _POSIX_C_SOURCE 200809L
#include "utils.h"
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>

char* parse_address(const char* address, uint16_t* port) {
    char* addr_copy = strdup(address);
    if (!addr_copy) {
        return NULL;
    }
    
    char* colon = strchr(addr_copy, ':');
    if (colon) {
        *colon = '\0';
        char* port_str = colon + 1;
        *port = (uint16_t)atoi(port_str);
        return addr_copy;
    }
    
    return addr_copy;
}

int set_socket_timeout(int fd, int timeout_sec) {
    struct timeval timeout;
    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;
    
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        return -1;
    }
    
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        return -1;
    }
    
    return 0;
}

int set_socket_reuse(int fd) {
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        return -1;
    }
    return 0;
}

const char* get_error_string(int errnum) {
    return strerror(errnum);
}

