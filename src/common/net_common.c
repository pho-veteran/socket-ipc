/**
 * @file net_common.c
 * @brief Common network utilities shared by server and client
 */

#include "net_common.h"
#include "error.h"
#include "logger.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int send_message(int fd, void* ssl, int is_ssl, const Message* msg) {
    // Serialize header
    MessageHeader header = msg->header;
    message_header_serialize(&header);
    
    // Send header
    ssize_t sent = 0;
    if (is_ssl && ssl) {
        sent = SSL_write((SSL*)ssl, &header, sizeof(header));
    } else {
        sent = send(fd, &header, sizeof(header), 0);
    }
    
    if (sent != sizeof(header)) {
        return -1;
    }
    
    // Send payload
    if (msg->header.length > 0 && msg->payload) {
        if (is_ssl && ssl) {
            sent = SSL_write((SSL*)ssl, msg->payload, msg->payload_size);
        } else {
            sent = send(fd, msg->payload, msg->payload_size, 0);
        }
        
        if (sent != (ssize_t)msg->payload_size) {
            return -1;
        }
    }
    
    return 0;
}

int receive_message(int fd, void* ssl, int is_ssl, Message* msg) {
    MessageHeader header;
    
    // Receive header
    ssize_t received = 0;
    if (is_ssl && ssl) {
        received = SSL_read((SSL*)ssl, &header, sizeof(header));
    } else {
        received = recv(fd, &header, sizeof(header), MSG_WAITALL);
    }
    
    if (received != sizeof(header)) {
        if (received == 0) {
            return -1; // Connection closed
        }
        return -1;
    }
    
    message_header_deserialize(&header);
    
    // Initialize message
    msg->header = header;
    msg->payload = NULL;
    msg->payload_size = 0;
    
    // Receive payload
    if (header.length > 0) {
        msg->payload = (uint8_t*)malloc(header.length);
        if (!msg->payload) {
            return -1;
        }
        msg->payload_size = header.length;
        
        if (is_ssl && ssl) {
            received = SSL_read((SSL*)ssl, msg->payload, header.length);
        } else {
            received = recv(fd, msg->payload, header.length, MSG_WAITALL);
        }
        
        if (received != (ssize_t)header.length) {
            free(msg->payload);
            msg->payload = NULL;
            msg->payload_size = 0;
            return -1;
        }
    }
    
    return 0;
}

void close_tls_connection(void* ssl) {
    if (ssl) {
        SSL_shutdown((SSL*)ssl);
        SSL_free((SSL*)ssl);
    }
}

void cleanup_tls(void* ssl_ctx) {
    if (ssl_ctx) {
        SSL_CTX_free((SSL_CTX*)ssl_ctx);
        EVP_cleanup();
    }
}

