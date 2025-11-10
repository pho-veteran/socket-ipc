/**
 * @file error.h
 * @brief Error handling definitions
 * 
 * Unified error handling system with error codes.
 */

#ifndef ERROR_H
#define ERROR_H

/**
 * @brief Error code enumeration
 */
typedef enum {
    ERROR_SUCCESS = 0,
    ERROR_NETWORK,      ///< Network/socket error
    ERROR_PROTOCOL,     ///< Protocol parsing/format error
    ERROR_TLS,          ///< TLS/SSL error
    ERROR_TIMEOUT,      ///< Operation timeout
    ERROR_CONNECTION,   ///< Connection failed/closed
    ERROR_INVALID_ARG,  ///< Invalid argument
    ERROR_SYSTEM        ///< System call error
} ErrorCode;

/**
 * @brief Convert error code to string
 * @param code Error code
 * @return String description
 */
const char* error_to_string(ErrorCode code);

#endif // ERROR_H

