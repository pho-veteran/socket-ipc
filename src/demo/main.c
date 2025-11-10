/**
 * @file demo/main.c
 * @brief Demo entry point
 */

#include "demo_runner.h"
#include "../common/types.h"
#include "../common/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    SocketMode mode = SOCKET_MODE_INET;
    const char* address = "localhost:8080";
    int use_tls = 0;
    
    // Simple argument parsing
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            if (strcmp(argv[i + 1], "unix") == 0) {
                mode = SOCKET_MODE_UNIX;
                address = "/tmp/server.sock";
            }
            i++;
        } else if (strcmp(argv[i], "--address") == 0 && i + 1 < argc) {
            address = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "--tls") == 0) {
            use_tls = 1;
        }
    }
    
    // Initialize logger
    if (logger_init(NULL) < 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }
    
    // Run demo
    int result = demo_run(mode, address, use_tls);
    
    logger_cleanup();
    return result;
}

