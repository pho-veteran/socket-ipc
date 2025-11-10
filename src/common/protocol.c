/**
 * @file protocol.c
 * @brief Protocol implementation
 */

#include "protocol.h"
#include <stdlib.h>
#include <string.h>

Message message_create_text(const char* text, size_t text_len) {
    Message msg;
    msg.header.type = MSG_TYPE_TEXT;
    msg.header.length = text_len;
    msg.header.flags = MSG_FLAGS_NONE;
    
    if (text_len > 0) {
        msg.payload = (uint8_t*)malloc(text_len);
        if (!msg.payload) {
            msg.payload_size = 0;
            return msg;
        }
        memcpy(msg.payload, text, text_len);
        msg.payload_size = text_len;
    } else {
        msg.payload = NULL;
        msg.payload_size = 0;
    }
    
    return msg;
}

Message message_create_ack(void) {
    Message msg;
    msg.header.type = MSG_TYPE_ACK;
    msg.header.length = 0;
    msg.header.flags = MSG_FLAGS_NONE;
    msg.payload = NULL;
    msg.payload_size = 0;
    return msg;
}

Message message_create_error(const char* error, size_t error_len) {
    Message msg;
    msg.header.type = MSG_TYPE_ERROR;
    msg.header.length = error_len;
    msg.header.flags = MSG_FLAGS_NONE;
    
    if (error_len > 0) {
        msg.payload = (uint8_t*)malloc(error_len);
        if (!msg.payload) {
            msg.payload_size = 0;
            return msg;
        }
        memcpy(msg.payload, error, error_len);
        msg.payload_size = error_len;
    } else {
        msg.payload = NULL;
        msg.payload_size = 0;
    }
    
    return msg;
}

void message_free(Message* msg) {
    if (msg && msg->payload) {
        free(msg->payload);
        msg->payload = NULL;
        msg->payload_size = 0;
    }
}

