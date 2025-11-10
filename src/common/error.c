/**
 * @file error.c
 * @brief Error handling implementation
 */

#include "error.h"

const char* error_to_string(ErrorCode code) {
    switch (code) {
        case ERROR_SUCCESS:
            return "Success";
        case ERROR_NETWORK:
            return "Network error";
        case ERROR_PROTOCOL:
            return "Protocol error";
        case ERROR_TLS:
            return "TLS/SSL error";
        case ERROR_TIMEOUT:
            return "Operation timeout";
        case ERROR_CONNECTION:
            return "Connection error";
        case ERROR_INVALID_ARG:
            return "Invalid argument";
        case ERROR_SYSTEM:
            return "System error";
        default:
            return "Unknown error";
    }
}

