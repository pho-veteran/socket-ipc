/**
 * @file demo_runner.h
 * @brief Demo orchestrator
 * 
 * Simulates multiple clients connecting/disconnecting and sending messages.
 */

#ifndef DEMO_RUNNER_H
#define DEMO_RUNNER_H

#include "../common/types.h"

/**
 * @brief Run demo
 * @param mode Socket mode
 * @param address Server address
 * @param use_tls Whether to use TLS
 * @return 0 on success, -1 on error
 */
int demo_run(SocketMode mode, const char* address, int use_tls);

#endif // DEMO_RUNNER_H

