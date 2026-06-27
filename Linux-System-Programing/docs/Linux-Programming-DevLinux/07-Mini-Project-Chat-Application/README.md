## Chat Application

A peer-to-peer chat application with command-line interface for remote messaging over TCP/IP.

### Overview

This application enables direct messaging between peers through TCP connections. It serves as both client and server, allowing users to initiate connections to other peers and accept incoming connections simultaneously.

### Features

- Command-line interface for easy interaction
- Connect to multiple peers simultaneously
- Exchange messages in real-time
- Terminate connections gracefully
- Thread-safe connection management
- Clean resource handling to prevent memory leaks

### Project Structure

```
.
├── bin/            # Binary executables
├── inc/            # Header files
│   ├── command.h   # Command processing definitions
│   ├── connection.h# Connection management
│   ├── message.h   # Message handling
│   └── utils.h     # Utility functions
├── log/            # Log files
├── obj/            # Compiled object files
├── src/            # Source code
│   ├── main.c      # Main application entry
│   ├── command.c   # Command processing
│   ├── connection.c# Connection handling
│   ├── message.c   # Message functions
│   └── utils.c     # Utility functions
├── Makefile        # Build configuration
└── README.md       # This file
```

### Building

To build the application, use the provided Makefile:

```bash
make
```

This will create the executable file at `bin/chat`.

### Running

To run the application, provide a port number as a command-line argument:

```bash
./bin/chat <port>
```

Example:
```bash
./bin/chat 8000
```

This starts the application and listens for incoming connections on port 8000.

### Commands

The application supports the following commands:

- `help` - Display all available commands
- `myip` - Display your IP address
- `myport` - Display your port number
- `connect <ip> <port>` - Connect to a peer
- `list` - List all active connections
- `terminate <id>` - Terminate a connection
- `send <id> <message>` - Send a message to a peer
- `exit` - Exit the application

### Example Usage

#### Starting the Application
```
$ ./bin/chat 8000
Chat application started on port 8000

-------- Command List --------
help                         : Display all commands
myip                         : Display your IP address
myport                       : Display your port number
connect <ip> <port>          : Connect to a peer
list                         : List all active connections
terminate <id>               : Terminate a connection
send <id> <message>          : Send a message to a peer
exit                         : Exit the application
-----------------------------

Enter command: 
```

#### Checking Your IP and Port
```
Enter command: myip
Your IP address: 192.168.1.5
Enter command: myport
Your port: 8000
```

#### Connecting to a Peer
```
Enter command: connect 192.168.1.10 8001
Connected to 192.168.1.10:8001
```

#### Listing Connections
```
Enter command: list

-------- Connection List --------
ID  |  IP Address        |  Port  |  Type
----------------------------------------
0   |  192.168.1.10      |  8001  |  Outgoing
1   |  192.168.1.15      |  8002  |  Incoming
----------------------------------------
Total: 2 connection(s)
```

#### Sending a Message
```
Enter command: send 0 Hello, how are you?
Message sent to connection 0.
```

#### Receiving a Message
```
***Message received from: 192.168.1.10
***Sender Port:          8001
-->Message:              Hi there! I'm doing well, thanks for asking.

Enter command: 
```

#### Terminating a Connection
```
Enter command: terminate 0
Connection 0 terminated successfully.
```

#### Exiting the Application
```
Enter command: exit
Exiting application...
Cleaning up resources...
All resources cleaned up.
```

### Memory Leak Testing

To check for memory leaks using Valgrind:

```bash
make valgrind
```

### Cleaning Up

To remove compiled files:

```bash
make clean
```

## Implementation Notes

- The application uses POSIX threads for handling multiple connections
- Each connection has a dedicated thread for receiving messages
- Thread safety is ensured using mutex locks for critical sections
- All threads are detached to avoid resource leaks
- Signals (SIGINT) are handled for clean program termination