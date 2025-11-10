/**
 * @file logger.h
 * @brief Thread-safe logging system
 * 
 * Provides logging functionality with different log levels
 * and thread-safe output to files.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdarg.h>

/**
 * @brief Log level enumeration
 */
typedef enum {
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel;

/**
 * @brief Initialize logger with output file
 * @param output_file File to write logs to (NULL for stdout)
 * @return 0 on success, -1 on error
 */
int logger_init(const char* output_file);

/**
 * @brief Cleanup logger
 */
void logger_cleanup(void);

/**
 * @brief Set log level
 */
void logger_set_level(LogLevel level);

/**
 * @brief Log a message
 */
void logger_log(LogLevel level, const char* format, ...);

/**
 * @brief Log a message (va_list version)
 */
void vlogger_log(LogLevel level, const char* format, va_list args);

/**
 * @brief Log info message
 */
void logger_info(const char* format, ...);

/**
 * @brief Log warning message
 */
void logger_warn(const char* format, ...);

/**
 * @brief Log error message
 */
void logger_error(const char* format, ...);

#endif // LOGGER_H

