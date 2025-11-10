/**
 * @file protocol.h
 * @brief Protocol definitions for client-server communication
 * 
 * Defines the message format and types used for communication
 * between client and server processes.
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <endian.h>

/**
 * @brief Message type enumeration
 */
typedef enum {
    MSG_TYPE_TEXT = 0x01,    ///< Text message
    MSG_TYPE_ACK = 0x03,     ///< Acknowledgment
    MSG_TYPE_ERROR = 0x04    ///< Error message
} MessageType;

/**
 * @brief Message flags
 */
typedef enum {
    MSG_FLAGS_NONE = 0x00,
    MSG_FLAGS_COMPRESSED = 0x01,  ///< Payload is compressed
    MSG_FLAGS_ENCRYPTED = 0x02,   ///< Payload is encrypted
    MSG_FLAGS_FINAL = 0x04        ///< Final message in sequence
} MessageFlags;

/**
 * @brief Message header structure (16 bytes)
 * 
 * Fixed-size header preceding all messages:
 * - type: 4 bytes (MessageType)
 * - length: 8 bytes (payload size)
 * - flags: 4 bytes (MessageFlags)
 */
typedef struct {
    uint32_t type;      ///< Message type
    uint64_t length;    ///< Payload length in bytes
    uint32_t flags;     ///< Message flags
} __attribute__((packed)) MessageHeader;

/**
 * @brief Complete message structure
 */
typedef struct {
    MessageHeader header;
    uint8_t* payload;   ///< Payload data (allocated dynamically)
    size_t payload_size; ///< Actual payload size
} Message;

/**
 * @brief Serialize header to network byte order
 */
static inline void message_header_serialize(MessageHeader* header) {
    header->type = htonl(header->type);
    header->length = htobe64(header->length);
    header->flags = htonl(header->flags);
}

/**
 * @brief Deserialize header from network byte order
 */
static inline void message_header_deserialize(MessageHeader* header) {
    header->type = ntohl(header->type);
    header->length = be64toh(header->length);
    header->flags = ntohl(header->flags);
}

/**
 * @brief Get total message size (header + payload)
 */
static inline size_t message_total_size(const Message* msg) {
    return sizeof(MessageHeader) + msg->payload_size;
}

/**
 * @brief Create a text message
 */
Message message_create_text(const char* text, size_t text_len);

/**
 * @brief Create an ACK message
 */
Message message_create_ack(void);

/**
 * @brief Create an error message
 */
Message message_create_error(const char* error, size_t error_len);

/**
 * @brief Free message payload
 */
void message_free(Message* msg);

#endif // PROTOCOL_H

