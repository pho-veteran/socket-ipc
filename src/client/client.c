/**
 * @file client.c
 * @brief Client implementation
 */

#define _POSIX_C_SOURCE 200809L
#include "client.h"
#include "client_net.h"
#include "../common/logger.h"
#include "../common/utils.h"
#include "../common/net_common.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

int client_init(Client* client, SocketMode mode, const char* address, int enable_tls) {
    memset(client, 0, sizeof(Client));
    client->mode = mode;
    client->address = strdup(address);
    if (!client->address) {
        return -1;
    }
    client->enable_tls = enable_tls;
    client->fd = -1;
    client->ssl = NULL;
    client->connected = 0;
    client->timeout_sec = 5;
    client->ssl_ctx = NULL;
    
    // Initialize TLS if enabled
    if (enable_tls) {
        client->ssl_ctx = init_tls_client();
        if (!client->ssl_ctx) {
            free(client->address);
            return -1;
        }
    }
    
    return 0;
}

void client_cleanup(Client* client) {
    if (!client) return;
    
    client_disconnect(client);
    
    if (client->ssl_ctx) {
        cleanup_tls(client->ssl_ctx);
    }
    
    free(client->address);
    memset(client, 0, sizeof(Client));
}

int client_connect(Client* client, int timeout_sec) {
    if (!client) return -1;
    
    client->timeout_sec = timeout_sec;
    int retry_count = 0;
    const int max_retries = 5;
    
    while (retry_count < max_retries) {
        if (client->mode == SOCKET_MODE_UNIX) {
            client->fd = connect_unix_socket(client->address);
        } else {
            uint16_t port = 8080;
            char* host = parse_address(client->address, &port);
            if (!host) {
                if (retry_count < max_retries - 1) {
                    int backoff = (int)pow(2, retry_count);
                    logger_warn("Connection failed, retrying in %d seconds...", backoff);
                    sleep(backoff);
                    retry_count++;
                    continue;
                }
                return -1;
            }
            client->fd = connect_inet_socket(host, port);
            free(host);
        }
        
        if (client->fd < 0) {
            if (retry_count < max_retries - 1) {
                int backoff = (int)pow(2, retry_count);
                logger_warn("Connection failed, retrying in %d seconds...", backoff);
                sleep(backoff);
                retry_count++;
                continue;
            }
            return -1;
        }
        
        if (set_socket_timeout(client->fd, client->timeout_sec) < 0) {
            close(client->fd);
            client->fd = -1;
            if (retry_count < max_retries - 1) {
                int backoff = (int)pow(2, retry_count);
                logger_warn("Connection failed, retrying in %d seconds...", backoff);
                sleep(backoff);
                retry_count++;
                continue;
            }
            return -1;
        }
        
        if (client->enable_tls) {
            client->ssl = connect_tls(client->fd, client->ssl_ctx);
            if (!client->ssl) {
                close(client->fd);
                client->fd = -1;
                if (retry_count < max_retries - 1) {
                    int backoff = (int)pow(2, retry_count);
                    logger_warn("TLS connection failed, retrying in %d seconds...", backoff);
                    sleep(backoff);
                    retry_count++;
                    continue;
                }
                return -1;
            }
        }
        
        client->connected = 1;
        logger_info("Connected to server at %s", client->address);
        return 0;
    }
    
    return -1;
}

void client_disconnect(Client* client) {
    if (!client || !client->connected) {
        return;
    }
    
    if (client->enable_tls && client->ssl) {
        close_tls_connection(client->ssl);
        client->ssl = NULL;
    }
    
    if (client->fd >= 0) {
        close(client->fd);
        client->fd = -1;
    }
    
    client->connected = 0;
    logger_info("Disconnected from server");
}

int client_is_connected(const Client* client) {
    return client && client->connected && client->fd >= 0;
}

int client_send_text(Client* client, const char* text, size_t text_len) {
    if (!client_is_connected(client)) {
        logger_error("Not connected to server");
        return -1;
    }
    
    Message msg = message_create_text(text, text_len);
    if (send_message(client->fd, client->ssl, client->enable_tls, &msg) < 0) {
        message_free(&msg);
        return -1;
    }
    
    // Wait for ACK
    Message ack;
    if (receive_message(client->fd, client->ssl, client->enable_tls, &ack) < 0) {
        message_free(&msg);
        return -1;
    }
    
    if (ack.header.type != MSG_TYPE_ACK) {
        message_free(&msg);
        message_free(&ack);
        logger_error("Expected ACK, got different message type");
        return -1;
    }
    
    message_free(&msg);
    message_free(&ack);
    return 0;
}

int client_receive_message(Client* client, Message* msg) {
    if (!client_is_connected(client)) {
        logger_error("Not connected to server");
        return -1;
    }
    
    return receive_message(client->fd, client->ssl, client->enable_tls, msg);
}

void client_set_timeout(Client* client, int timeout_sec) {
    if (!client) return;
    
    client->timeout_sec = timeout_sec;
    if (client->fd >= 0) {
        set_socket_timeout(client->fd, client->timeout_sec);
    }
}

