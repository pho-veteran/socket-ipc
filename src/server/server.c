/**
 * @file server.c
 * @brief Server implementation
 */

#define _POSIX_C_SOURCE 200809L
#include "server.h"
#include "server_net.h"
#include "../common/logger.h"
#include "../common/utils.h"
#include "../common/net_common.h"
#include <signal.h>
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <sys/un.h>

static void add_client(Server* server, int fd, void* ssl) {
    if (server->client_count >= server->client_capacity) {
        size_t new_capacity = server->client_capacity == 0 ? 8 : server->client_capacity * 2;
        ClientConnection* new_clients = realloc(server->clients, new_capacity * sizeof(ClientConnection));
        if (!new_clients) {
            logger_error("Failed to allocate memory for clients");
            return;
        }
        server->clients = new_clients;
        server->client_capacity = new_capacity;
    }
    
    server->clients[server->client_count].fd = fd;
    server->clients[server->client_count].ssl = ssl;
    server->clients[server->client_count].is_ssl = (ssl != NULL);
    server->client_count++;
    server->total_clients++;
}

static void remove_client(Server* server, size_t index) {
    if (index >= server->client_count) {
        return;
    }
    
    ClientConnection* client = &server->clients[index];
    logger_info("Client disconnected (fd=%d)", client->fd);
    
    if (client->is_ssl && client->ssl) {
        close_tls_connection(client->ssl);
    }
    close(client->fd);
    
    // Move remaining clients
    for (size_t i = index; i < server->client_count - 1; i++) {
        server->clients[i] = server->clients[i + 1];
    }
    server->client_count--;
}

static void accept_new_connection(Server* server) {
    int client_fd = accept(server->server_fd, NULL, NULL);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            logger_error("Accept error: %s", get_error_string(errno));
        }
        return;
    }
    
    void* ssl = NULL;
    if (server->enable_tls) {
        ssl = accept_tls_connection(client_fd, server->ssl_ctx);
        if (!ssl) {
            close(client_fd);
            return;
        }
    }
    
    add_client(server, client_fd, ssl);
    logger_info("New client connected (fd=%d)", client_fd);
}

static void handle_client_message(Server* server, size_t client_index, MessageHandler handler) {
    ClientConnection* client = &server->clients[client_index];
    
    Message msg;
    if (receive_message(client->fd, client->ssl, client->is_ssl, &msg) < 0) {
        remove_client(server, client_index);
        return;
    }
    
    // Record metrics
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double now = tv.tv_sec + tv.tv_usec / 1000000.0;
    if (server->last_message_time > 0.0) {
        double interval_ms = (now - server->last_message_time) * 1000.0;
        server->total_interval_ms += interval_ms;
        if (server->interval_count == 0 || interval_ms < server->min_interval_ms) {
            server->min_interval_ms = interval_ms;
        }
        if (server->interval_count == 0 || interval_ms > server->max_interval_ms) {
            server->max_interval_ms = interval_ms;
        }
        server->interval_count++;
    }
    server->last_message_time = now;
    server->total_messages++;
    server->total_bytes += msg.payload_size;
    
    // Calculate latency if we have timing info (simplified - would need to track send time)
    // For now, we'll use a simple approximation
    
    if (handler) {
        handler(client->fd, &msg);
    }
    
    // Echo ACK for non-ACK messages
    if (msg.header.type != MSG_TYPE_ACK) {
        Message ack = message_create_ack();
        send_message(client->fd, client->ssl, client->is_ssl, &ack);
        message_free(&ack);
    }
    
    message_free(&msg);
}

static void run_event_loop(Server* server, MessageHandler handler) {
    struct pollfd* pollfds = NULL;
    size_t pollfd_count = 0;
    size_t pollfd_capacity = 0;
    
    while (server->running) {
        // Reallocate pollfds if needed
        size_t needed = 1 + server->client_count; // server + clients
        if (needed > pollfd_capacity) {
            size_t new_capacity = needed + 8;
            struct pollfd* new_pollfds = realloc(pollfds, new_capacity * sizeof(struct pollfd));
            if (!new_pollfds) {
                logger_error("Failed to allocate memory for pollfds");
                break;
            }
            pollfds = new_pollfds;
            pollfd_capacity = new_capacity;
        }
        
        pollfd_count = 0;
        
        // Add server socket
        pollfds[pollfd_count].fd = server->server_fd;
        pollfds[pollfd_count].events = POLLIN;
        pollfd_count++;
        
        // Add client sockets
        for (size_t i = 0; i < server->client_count; i++) {
            pollfds[pollfd_count].fd = server->clients[i].fd;
            pollfds[pollfd_count].events = POLLIN;
            pollfd_count++;
        }
        
        int poll_result = poll(pollfds, pollfd_count, 1000);
        
        if (poll_result < 0) {
            if (errno == EINTR) continue;
            logger_error("Poll error: %s", get_error_string(errno));
            break;
        }
        
        if (poll_result == 0) continue;  // Timeout
        
        // Check server socket for new connections
        if (pollfds[0].revents & POLLIN) {
            accept_new_connection(server);
        }
        
        // Check client sockets
        for (size_t i = 1; i < pollfd_count; i++) {
            size_t client_index = i - 1;
            if (client_index >= server->client_count) continue;
            
            if (pollfds[i].revents & POLLIN) {
                handle_client_message(server, client_index, handler);
            }
            if (pollfds[i].revents & (POLLHUP | POLLERR)) {
                remove_client(server, client_index);
                // Adjust indices after removal
                i--;
                pollfd_count--;
            }
        }
    }
    
    free(pollfds);
}

int server_init(Server* server, SocketMode mode, const char* address, int enable_tls) {
    memset(server, 0, sizeof(Server));
    server->mode = mode;
    server->address = strdup(address);
    if (!server->address) {
        return -1;
    }
    server->enable_tls = enable_tls;
    server->server_fd = -1;
    server->running = 0;
    server->clients = NULL;
    server->client_count = 0;
    server->client_capacity = 0;
    server->total_clients = 0;
    server->total_messages = 0;
    server->total_bytes = 0;
    server->last_message_time = 0.0;
    server->total_interval_ms = 0.0;
    server->min_interval_ms = 0.0;
    server->max_interval_ms = 0.0;
    server->interval_count = 0;
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    server->start_time = tv.tv_sec + tv.tv_usec / 1000000.0;
    
    // Initialize TLS if enabled
    if (enable_tls) {
        server->ssl_ctx = init_tls_server();
        if (!server->ssl_ctx) {
            free(server->address);
            return -1;
        }
    } else {
        server->ssl_ctx = NULL;
    }
    
    return 0;
}

void server_cleanup(Server* server) {
    if (!server) return;
    
    server->running = 0;
    
    // Close all client connections
    for (size_t i = 0; i < server->client_count; i++) {
        ClientConnection* client = &server->clients[i];
        if (client->is_ssl && client->ssl) {
            close_tls_connection(client->ssl);
        }
        close(client->fd);
    }
    free(server->clients);
    
    // Close server socket
    if (server->server_fd >= 0) {
        close(server->server_fd);
        if (server->mode == SOCKET_MODE_UNIX) {
            unlink(server->address);
        }
    }
    
    // Cleanup TLS
    if (server->ssl_ctx) {
        cleanup_tls(server->ssl_ctx);
    }
    
    free(server->address);
    memset(server, 0, sizeof(Server));
}

static void setup_socket(Server* server) {
    if (server->mode == SOCKET_MODE_UNIX) {
        logger_info("Initializing UNIX socket at %s", server->address);
        server->server_fd = setup_unix_socket(server->address);
    } else {
        uint16_t port = 8080;
        char* host = parse_address(server->address, &port);
        if (!host) {
            logger_error("Failed to parse address");
            return;
        }
        logger_info("Initializing INET socket host=%s port=%u", host, port);
        server->server_fd = setup_inet_socket(host, port);
        free(host);
    }
    
    if (server->server_fd < 0) {
        return;
    }
    
    // Set non-blocking
    int flags = fcntl(server->server_fd, F_GETFL, 0);
    fcntl(server->server_fd, F_SETFL, flags | O_NONBLOCK);
}

int server_start(Server* server, MessageHandler handler) {
    setup_socket(server);
    if (server->server_fd < 0) {
        return -1;
    }
    
    if (listen(server->server_fd, SOMAXCONN) < 0) {
        logger_error("Failed to listen: %s", get_error_string(errno));
        return -1;
    }
    
    server->running = 1;
    if (server->mode == SOCKET_MODE_UNIX) {
        logger_info("Server listening (UNIX) at path=%s", server->address);
    } else {
        logger_info("Server listening (INET) at address=%s", server->address);
    }
    run_event_loop(server, handler);
    
    return 0;
}

void server_stop(Server* server) {
    if (server) {
        server->running = 0;
    }
}

size_t server_get_client_count(const Server* server) {
    return server ? server->client_count : 0;
}

void server_get_metrics(const Server* server,
                       size_t* total_clients,
                       size_t* total_messages,
                       double* uptime,
                       double* throughput_mbps,
                       double* avg_latency_ms,
                       double* min_latency_ms,
                       double* max_latency_ms) {
    if (!server) return;
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double current_time = tv.tv_sec + tv.tv_usec / 1000000.0;
    
    if (total_clients) *total_clients = server->total_clients;
    if (total_messages) *total_messages = server->total_messages;
    if (uptime) *uptime = current_time - server->start_time;
    
    // Calculate throughput (MB/s)
    if (throughput_mbps) {
        if (uptime && *uptime > 0.0) {
            double megabytes = server->total_bytes / (1024.0 * 1024.0);
            *throughput_mbps = megabytes / *uptime;
        } else {
            *throughput_mbps = 0.0;
        }
    }
    
    // Message interval statistics (ms)
    if (avg_latency_ms) {
        if (server->interval_count > 0) {
            *avg_latency_ms = server->total_interval_ms / server->interval_count;
        } else {
            *avg_latency_ms = 0.0;
        }
    }
    if (min_latency_ms) {
        *min_latency_ms = (server->interval_count > 0) ? server->min_interval_ms : 0.0;
    }
    if (max_latency_ms) {
        *max_latency_ms = (server->interval_count > 0) ? server->max_interval_ms : 0.0;
    }
}

