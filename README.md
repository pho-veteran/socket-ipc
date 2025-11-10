# Client-Server System

A modular client-server system in C supporting AF_UNIX and AF_INET sockets, TLS encryption, and multi-client handling with file-based configuration.

## Features

- **Dual Socket Modes**: AF_UNIX (Unix domain sockets) and AF_INET (Internet sockets)
- **Multi-Client Support**: Handles multiple clients using poll() for I/O multiplexing
- **TLS Encryption**: Optional TLS/SSL support using OpenSSL
- **Custom Protocol**: Header/payload protocol for text messages
- **File-Based Configuration**: Server and client read configuration from input files
- **File-Based Logging**: Server and client write logs and status to output files
- **Metrics Collection**: Server tracks and reports statistics at shutdown

## Requirements

- C11 compatible compiler (gcc 5+ or clang 5+)
- OpenSSL development libraries
- Make

### Installation on Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install build-essential libssl-dev
```

### Installation on Arch Linux

```bash
sudo pacman -S base-devel openssl
```

## Building

```bash
make
```

This creates two executables at the project root:
- `run_server` – Server program
- `run_client` – Client program

## Usage

### Server

The server reads configuration from `server_input.txt` and writes logs to `server_output.txt`.

1. Create `server_input.txt` with the following format:
```
mode=inet
address=localhost:8080
tls=0
```

2. Run the server:
```bash
./run_server
```

3. The server will:
   - Read configuration from `server_input.txt`
   - Write real-time logs to `server_output.txt`
   - Append metrics at shutdown to `server_output.txt`

#### Server Input File Format

Key-value pairs, one per line:
- `mode`: Socket mode (`unix` or `inet`)
- `address`: Server address
  - For `unix`: socket file path (e.g., `/tmp/server.sock`)
  - For `inet`: host:port (e.g., `localhost:8080`)
- `tls`: Enable TLS (1 for yes, 0 for no)

#### Server Output

The server writes logs to `server_output.txt` in real-time. At shutdown, it appends metrics:

```
=== SERVER METRICS ===
Mode: inet
Address: localhost:8080
TLS Enabled: No
Total Clients: 5
Total Messages Received: 42
Uptime: 123.45 seconds
Throughput: 1.23 Mbps
Average Latency: 12.34 ms
Min Latency: 8.90 ms
Max Latency: 25.67 ms
```

### Client

The client reads configuration and messages from `client_input.txt` and writes status to `client_output.txt`.

1. Create `client_input.txt` with the following format:
```
mode=inet
address=localhost:8080
tls=0
message=Hello World
message=Test Message
message=Another message
```

2. Run the client:
```bash
./run_client
```

3. The client will:
   - Read configuration and messages from `client_input.txt`
   - Connect to the server
   - Send all messages
   - Write status to `client_output.txt`

#### Client Input File Format

Key-value pairs, one per line:
- `mode`: Socket mode (`unix` or `inet`)
- `address`: Server address
- `tls`: Enable TLS (1 for yes, 0 for no)
- `message`: Messages to send (one per line, can have multiple)

#### Client Output

The client writes connection status and send status to `client_output.txt`.


## Examples

### Example 1: Basic Server-Client (INET)

**Terminal 1 (Server):**

Create `server_input.txt`:
```
mode=inet
address=localhost:8080
tls=0
```

Run server:
```bash
./server
```

**Terminal 2 (Client):**

Create `client_input.txt`:
```
mode=inet
address=localhost:8080
tls=0
message=Hello from client
message=Test message
```

Run client:
```bash
./client
```

### Example 2: Unix Domain Socket

**Server `server_input.txt`:**
```
mode=unix
address=/tmp/server.sock
tls=0
```

**Client `client_input.txt`:**
```
mode=unix
address=/tmp/server.sock
tls=0
message=Hello via Unix socket
```

### Example 3: TLS Communication

**Server `server_input.txt`:**
```
mode=inet
address=localhost:8443
tls=1
```

**Client `client_input.txt`:**
```
mode=inet
address=localhost:8443
tls=1
message=Secure message
```

## Protocol Specification

### Message Header

All messages begin with a fixed 16-byte header:

```
Offset  Size    Field
------  ----    -----
0       4       Message Type (uint32_t)
4       8       Payload Length (uint64_t)
12      4       Flags (uint32_t)
```

### Message Types

- `TEXT` (0x01): Text message
- `ACK` (0x03): Acknowledgment
- `ERROR` (0x04): Error message

### Message Flow

1. Client sends message with header + payload
2. Server receives and processes message
3. Server sends ACK message
4. Client receives ACK and continues

## Architecture

### Project Structure

```
src/
 ├── server/
 │    ├── main.c              # Server entry point
 │    ├── server.c/h          # Server implementation
 │    └── server_net.c/h      # Server network layer
 ├── client/
 │    ├── main.c              # Client entry point
 │    ├── client.c/h          # Client implementation
 │    └── client_net.c/h      # Client network layer
 ├── common/
 │    ├── protocol.h/c        # Protocol definitions
 │    ├── types.h             # Common types
 │    ├── error.c/h           # Error handling
 │    ├── logger.c/h          # Logging system
 │    ├── net_common.c/h      # Network utilities
 │    └── utils.c/h          # Utility functions
 └── demo/
      ├── main.c              # Demo entry point
      └── demo_runner.c/h     # Demo orchestrator
```

### Key Components

- **Server**: Multi-client server using poll() for I/O multiplexing
- **Client**: Client with automatic reconnection and timeout handling
- **Protocol**: Custom binary protocol with header/payload structure
- **TLS**: OpenSSL integration for secure communication
- **Logger**: Thread-safe file-based logging system

## Error Handling

The system uses error codes:
- `ERROR_SUCCESS`: Success
- `ERROR_NETWORK`: Network/socket error
- `ERROR_PROTOCOL`: Protocol parsing/format error
- `ERROR_TLS`: TLS/SSL error
- `ERROR_TIMEOUT`: Operation timeout
- `ERROR_CONNECTION`: Connection failed/closed
- `ERROR_INVALID_ARG`: Invalid argument
- `ERROR_SYSTEM`: System call error

## Logging

Thread-safe logging system with three levels:
- `INFO`: Informational messages
- `WARN`: Warning messages
- `ERROR`: Error messages

Logs are timestamped and written to files (server_output.txt or client_output.txt).

## TLS/SSL

TLS support is implemented using OpenSSL:
- Self-signed certificates generated for demo purposes
- TLS 1.2+ protocol support
- Server and client TLS contexts
- Secure send/receive wrappers

For production use, provide proper certificates via configuration.

## Cleanup

To clean build artifacts and output files:

```bash
make clean
```

This removes:
- Object files
- Executables
- Test files
- Input/output files (server_input.txt, server_output.txt, client_input.txt, client_output.txt)

## Troubleshooting

### Build Errors

- **OpenSSL not found**: Install `libssl-dev` (Ubuntu) or `openssl` (Arch)
- **Compilation errors**: Ensure C11 support (`-std=c11`)

### Runtime Errors

- **Permission denied (Unix socket)**: Check socket file permissions
- **Address already in use**: Kill existing server or use different port/path
- **TLS handshake failed**: Verify certificate configuration
- **File not found**: Ensure `server_input.txt` or `client_input.txt` exists

## Performance Notes

- AF_UNIX sockets are typically faster for local IPC
- AF_INET sockets enable network communication
- TLS adds overhead but provides security
- Metrics are calculated at server shutdown

## License

This project is provided as-is for educational purposes.
