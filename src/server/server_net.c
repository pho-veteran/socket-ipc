/**
 * @file server_net.c
 * @brief Server network layer implementation
 */

#define _DEFAULT_SOURCE
#include "server_net.h"
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
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

int setup_unix_socket(const char* socket_path) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        logger_error("Failed to create Unix socket: %s", get_error_string(errno));
        return -1;
    }
    
    // Remove existing socket file
    unlink(socket_path);
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        logger_error("Failed to bind Unix socket: %s", get_error_string(errno));
        return -1;
    }
    
    if (set_socket_reuse(fd) < 0) {
        close(fd);
        return -1;
    }
    
    return fd;
}

int setup_inet_socket(const char* host, uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        logger_error("Failed to create INET socket: %s", get_error_string(errno));
        return -1;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (!host || strlen(host) == 0 || strcmp(host, "localhost") == 0 || strcmp(host, "127.0.0.1") == 0) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_aton(host, &addr.sin_addr) == 0) {
            close(fd);
            logger_error("Invalid IP address: %s", host);
            return -1;
        }
    }
    
    if (set_socket_reuse(fd) < 0) {
        close(fd);
        return -1;
    }
    
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        logger_error("Failed to bind INET socket: %s", get_error_string(errno));
        return -1;
    }
    
    return fd;
}

void* init_tls_server(void) {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    
    const SSL_METHOD* method = TLS_server_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    
    if (!ctx) {
        logger_error("Failed to create SSL context");
        return NULL;
    }
    
    // Generate self-signed certificate for demo (modern EVP API)
    EVP_PKEY* pkey = NULL;
    EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (!pctx) {
        SSL_CTX_free(ctx);
        logger_error("Failed to create EVP_PKEY_CTX");
        return NULL;
    }
    if (EVP_PKEY_keygen_init(pctx) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        SSL_CTX_free(ctx);
        logger_error("EVP_PKEY_keygen_init failed");
        return NULL;
    }
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(pctx, 2048) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        SSL_CTX_free(ctx);
        logger_error("EVP_PKEY_CTX_set_rsa_keygen_bits failed");
        return NULL;
    }
    if (EVP_PKEY_keygen(pctx, &pkey) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        SSL_CTX_free(ctx);
        logger_error("EVP_PKEY_keygen failed");
        return NULL;
    }
    EVP_PKEY_CTX_free(pctx);
    
    X509* x509 = X509_new();
    if (!x509) {
        EVP_PKEY_free(pkey);
        SSL_CTX_free(ctx);
        logger_error("Failed to allocate X509");
        return NULL;
    }
    X509_set_version(x509, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), 31536000L);
    X509_set_pubkey(x509, pkey);
    
    X509_NAME* name = X509_get_subject_name(x509);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, 
                                (const unsigned char*)"localhost", -1, -1, 0);
    X509_set_issuer_name(x509, name);
    if (!X509_sign(x509, pkey, EVP_sha256())) {
        X509_free(x509);
        EVP_PKEY_free(pkey);
        SSL_CTX_free(ctx);
        logger_error("X509_sign failed");
        return NULL;
    }
    
    if (SSL_CTX_use_certificate(ctx, x509) != 1) {
        X509_free(x509);
        EVP_PKEY_free(pkey);
        SSL_CTX_free(ctx);
        logger_error("SSL_CTX_use_certificate failed");
        return NULL;
    }
    if (SSL_CTX_use_PrivateKey(ctx, pkey) != 1) {
        X509_free(x509);
        EVP_PKEY_free(pkey);
        SSL_CTX_free(ctx);
        logger_error("SSL_CTX_use_PrivateKey failed");
        return NULL;
    }
    
    X509_free(x509);
    EVP_PKEY_free(pkey);
    
    return ctx;
}

void* accept_tls_connection(int fd, void* ssl_ctx) {
    SSL_CTX* ctx = (SSL_CTX*)ssl_ctx;
    SSL* ssl = SSL_new(ctx);
    if (!ssl) {
        return NULL;
    }
    
    SSL_set_fd(ssl, fd);
    
    if (SSL_accept(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return NULL;
    }
    
    return ssl;
}

