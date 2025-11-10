/**
 * @file logger.c
 * @brief Logger implementation
 */

#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>

static FILE* log_file = NULL;
static LogLevel log_level = LOG_LEVEL_INFO;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

int logger_init(const char* output_file) {
    pthread_mutex_lock(&log_mutex);
    
    if (output_file) {
        log_file = fopen(output_file, "w");
        if (!log_file) {
            pthread_mutex_unlock(&log_mutex);
            return -1;
        }
    } else {
        log_file = stdout;
    }
    
    pthread_mutex_unlock(&log_mutex);
    return 0;
}

void logger_cleanup(void) {
    pthread_mutex_lock(&log_mutex);
    
    if (log_file && log_file != stdout) {
        fclose(log_file);
    }
    log_file = NULL;
    
    pthread_mutex_unlock(&log_mutex);
}

void logger_set_level(LogLevel level) {
    pthread_mutex_lock(&log_mutex);
    log_level = level;
    pthread_mutex_unlock(&log_mutex);
}

static const char* level_to_string(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_INFO: return "INFO";
        case LOG_LEVEL_WARN: return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void vlogger_log(LogLevel level, const char* format, va_list args) {
    if (level < log_level) return;
    
    pthread_mutex_lock(&log_mutex);
    
    if (!log_file) {
        log_file = stdout;
    }
    
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    fprintf(log_file, "[%s] [%s] ", time_str, level_to_string(level));
    vfprintf(log_file, format, args);
    fprintf(log_file, "\n");
    fflush(log_file);
    
    pthread_mutex_unlock(&log_mutex);
}

void logger_log(LogLevel level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlogger_log(level, format, args);
    va_end(args);
}

void logger_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlogger_log(LOG_LEVEL_INFO, format, args);
    va_end(args);
}

void logger_warn(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlogger_log(LOG_LEVEL_WARN, format, args);
    va_end(args);
}

void logger_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlogger_log(LOG_LEVEL_ERROR, format, args);
    va_end(args);
}

