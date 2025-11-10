/**
 * @file types.h
 * @brief Common type definitions
 */

#ifndef TYPES_H
#define TYPES_H

/**
 * @brief Socket mode enumeration
 */
typedef enum {
    SOCKET_MODE_UNIX,   ///< AF_UNIX (Unix domain socket)
    SOCKET_MODE_INET    ///< AF_INET (Internet socket)
} SocketMode;

#endif // TYPES_H

