/**
 * @file client_net.c
 * @brief Client network layer implementation
 */

#define _DEFAULT_SOURCE
#include "client_net.h"
#include "../common/error.h"
#include "../common/logger.h"
#include "../common/utils.h"
#include "../common/net_common.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/evp.h>

int connect_unix_socket(const char* socket_path) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        logger_error("Failed to create Unix socket: %s", get_error_string(errno));
        return -1;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        logger_error("Failed to connect to Unix socket: %s", get_error_string(errno));
        return -1;
    }
    
    return fd;
}

int connect_inet_socket(const char* host, uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        logger_error("Failed to create INET socket: %s", get_error_string(errno));
        return -1;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    // Handle localhost
    const char* host_ip = host;
    if (strcmp(host, "localhost") == 0) {
        host_ip = "127.0.0.1";
    }
    
    if (inet_aton(host_ip, &addr.sin_addr) == 0) {
        close(fd);
        logger_error("Invalid IP address: %s", host);
        return -1;
    }
    
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        logger_error("Failed to connect to INET socket: %s", get_error_string(errno));
        return -1;
    }
    
    return fd;
}

void* init_tls_client(void) {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    
    const SSL_METHOD* method = TLS_client_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    
    if (!ctx) {
        logger_error("Failed to create SSL context");
        return NULL;
    }
    
    // For demo, don't verify certificate
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
    
    return ctx;
}

void* connect_tls(int fd, void* ssl_ctx) {
    SSL_CTX* ctx = (SSL_CTX*)ssl_ctx;
    SSL* ssl = SSL_new(ctx);
    if (!ssl) {
        return NULL;
    }
    
    SSL_set_fd(ssl, fd);
    
    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return NULL;
    }
    
    return ssl;
}

