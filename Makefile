# Makefile for Client-Server System (C version)
CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread -std=c11
LDFLAGS = -pthread -lssl -lcrypto -lm

# Directories
SRC_DIR = src
COMMON_DIR = $(SRC_DIR)/common
SERVER_DIR = $(SRC_DIR)/server
CLIENT_DIR = $(SRC_DIR)/client

# Include directories
INCLUDES = -I$(SRC_DIR)

# Common sources
COMMON_SOURCES = $(COMMON_DIR)/error.c \
                 $(COMMON_DIR)/logger.c \
                 $(COMMON_DIR)/utils.c \
                 $(COMMON_DIR)/net_common.c \
                 $(COMMON_DIR)/protocol.c

# Server sources
SERVER_SOURCES = $(SERVER_DIR)/server.c \
                 $(SERVER_DIR)/server_net.c

# Client sources
CLIENT_SOURCES = $(CLIENT_DIR)/client.c \
                 $(CLIENT_DIR)/client_net.c

# Object files
COMMON_OBJECTS = $(COMMON_SOURCES:.c=.o)
SERVER_OBJECTS = $(SERVER_SOURCES:.c=.o)
CLIENT_OBJECTS = $(CLIENT_SOURCES:.c=.o)

# Executables
SERVER_TARGET = run_server
CLIENT_TARGET = run_client

# Default target
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# Server executable
$(SERVER_TARGET): $(COMMON_OBJECTS) $(SERVER_OBJECTS) $(SERVER_DIR)/main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Client executable
$(CLIENT_TARGET): $(COMMON_OBJECTS) $(CLIENT_OBJECTS) $(CLIENT_DIR)/main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Common objects
$(COMMON_DIR)/%.o: $(COMMON_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Server objects
$(SERVER_DIR)/%.o: $(SERVER_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Client objects
$(CLIENT_DIR)/%.o: $(CLIENT_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean
clean:
	rm -f $(COMMON_OBJECTS) $(SERVER_OBJECTS) $(CLIENT_OBJECTS)
	rm -f $(SERVER_DIR)/main.o $(CLIENT_DIR)/main.o
	rm -f run_server run_client server client
	rm -f test*.txt test*.bin
	rm -f server_output.txt client_output.txt
	find . -name "*.o" -delete

# Phony targets
.PHONY: all clean
