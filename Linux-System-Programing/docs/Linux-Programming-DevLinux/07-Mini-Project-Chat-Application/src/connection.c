/**
 * connection.c - Connection management implementation
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <sys/socket.h>
 #include <arpa/inet.h>
 #include <netinet/in.h>
 #include <pthread.h>
 #include <errno.h>
 #include "connection.h"
 #include "message.h"
 #include "utils.h"

 // Array of connections - both outgoing and incoming
 static connection_t connections[MAX_CONNECTIONS] = {0};

 // Sever information
 static int server_socket = -1;
 static int server_port = -1;
 static char server_ip[IP_LENGTH] = {0};

 // Thread for accepting incomming connections
 static pthread_t listener_thread;
 
 // Mutex for thread safety when accessing connection array
 static pthread_mutex_t conn_mutex = PTHREAD_MUTEX_INITIALIZER;

 // Next available connection ID
 static int next_conn_id = 0;

 // Connection counters
 static int active_connections = 0;

 // Local function prototypes
 static void* connection_listener(void* arg);
 static int find_free_slot(void);
 static int check_duplicate_connection(const char* ip, int port);

 int initialize_server(int port) {
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        print_error("Socket creation failed");
        return -1;
    }

    struct sockaddr_in server_addr;
    int opt = 1;
    // Set socket options
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        print_error("setsockopt failed");
        close(server_socket);
        server_socket = -1;
        return -1;
    }

    // Prepare server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind socket to address
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        print_error("Bind failed");
        close(server_socket);
        server_socket = -1;
        return -1;
    }
    
    // Start listening
    if (listen(server_socket, 5) < 0) {
        print_error("Listen failed");
        close(server_socket);
        server_socket = -1;
        return -1;
    }
    
    // Store server port
    server_port = port;

    // Get and store server IP
    if (!get_local_ip(server_ip, IP_LENGTH)) {
        strcpy(server_ip, "127.0.0.1");
    }

    // Initialize connection array
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        connections[i].socket = -1;
        connections[i].is_active = false;
    }

    return 0;
 }

 int get_listening_port(void) {
    return server_port;
 }
 
 int get_server_socket(void) {
    return server_socket;
 }

 int start_connection_listener(void) {
    if (pthread_create(&listener_thread, NULL, connection_listener, NULL) != 0) {
        print_error("Failed to create listener thread");
        return -1;
    }
    
    // Detach thread so we don't need to join it later
    pthread_detach(listener_thread);
    
    return 0;
 }

 static void* connection_listener(void* arg) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_socket;

    while (1) {
        // Accept new conncection
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);

        // Check if server socket was closed
        if (server_socket < 0) {
            break;
        }

        // Check for accept errors
        if (client_socket < 0) {
            if (errno != EINTR) {// Ignore if interrupted by signal
                print_error("Accept faield");
            }
            continue;
        }

        // Find a free slot for the new connection
        pthread_mutex_lock(&conn_mutex);
        int slot = find_free_slot();

        if (slot < 0) {
            pthread_mutex_unlock(&conn_mutex);
            print_error("Maximum connections reached");
            close(client_socket);
            continue;
        }

        // Initialize connection structure
        connections[slot].socket = client_socket;
        connections[slot].id = next_conn_id++;
        connections[slot].port = ntohs(client_addr.sin_port);
        connections[slot].addr = client_addr;
        connections[slot].is_active = true;
        connections[slot].is_incoming = true;

        // Covert IP address to string
        inet_ntop(AF_INET, &client_addr.sin_addr, connections[slot].ip, IP_LENGTH);

        active_connections++;
        pthread_mutex_unlock(&conn_mutex);

        printf("\nNew connection from %s:%d\n", connections[slot].ip, connections[slot].port);
        printf("Enter command: ");
        fflush(stdout);

        // Create a new thread to handle message from this connection
        connection_t *conn_copy = malloc(sizeof(connection_t));
        if (!conn_copy) {
            print_error("Memory allocation failed");
            pthread_mutex_lock(&conn_mutex);
            connections[slot].is_active = false;
            connections[slot].socket = -1;
            active_connections--;
            pthread_mutex_unlock(&conn_mutex);
            close(client_socket);
            continue;
        }
        
        // Copy connection data to avoid race conditions
        memcpy(conn_copy, &connections[slot], sizeof(connection_t));
        
        // Create thread for receiving messages
        if (pthread_create(&connections[slot].thread, NULL, receive_message_handler, conn_copy) != 0) {
            print_error("Failed to create receive thread");
            free(conn_copy);
            pthread_mutex_lock(&conn_mutex);
            connections[slot].is_active = false;
            connections[slot].socket = -1;
            active_connections--;
            pthread_mutex_unlock(&conn_mutex);
            close(client_socket);
            continue;
        }

        // Detach thread
        pthread_detach(connections[slot].thread);
    }
    
    return NULL;
 }

 int connect_to_peer(const char *ip, int port) {
    // Validate IP and port
    if (!is_valid_ip(ip)) {
        print_error("Invalid IP address");
        return -1;
    }

    if (port <= 0 || port > 65535) {
        print_error("Invalid port number");
        return -1;
    }

    // Check for self-connection
    if (is_same_address(ip, port, server_ip, server_port)) {
        print_error("Cannot connect to yourself");
        return -1;
    }

    // Check for duplicate connection
    pthread_mutex_lock(&conn_mutex);
    int dup_idx = check_duplicate_connection(ip, port);
    if (dup_idx >= 0) {
        pthread_mutex_unlock(&conn_mutex);
        print_error("Duplicate connection is not allowed");
        return -1;
    }

    // Find a free slot for the new connection
    int slot = find_free_slot();
    if (slot < 0) {
        pthread_mutex_unlock(&conn_mutex);
        print_error("Maximum connection reached");
        return -1;
    }
    pthread_mutex_unlock(&conn_mutex);

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        print_error("Socket creation faield");
        return -1;
    }

    // Prepare peer addresss
    struct sockaddr_in peer_addr;
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &peer_addr.sin_addr) < 0) {
        print_error("Invalid address");
        close(sock);
        return -1;
    }

    // Connect to peer
    if (connect(sock, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) < 0) {
        print_error("Connection failed");
        close(sock);
        return -1;
    }

    // Initalize connection structure
    pthread_mutex_lock(&conn_mutex);
    connections[slot].socket = sock;
    connections[slot].id = next_conn_id++;
    connections[slot].port = port;
    connections[slot].addr = peer_addr;
    connections[slot].is_active = true;
    connections[slot].is_incoming = false;
    strncpy(connections[slot].ip, ip, IP_LENGTH -1);
    connections[slot].ip[IP_LENGTH - 1] = '\0';

    active_connections++;
    pthread_mutex_unlock(&conn_mutex);

    printf("Connected to %s:%d\n", ip, port);

    // Create a new thread to handle message from this connection
    connection_t *conn_copy = malloc(sizeof(sizeof(connection_t)));
    if (!conn_copy) {
        print_error("Memory allocation failed");
        pthread_mutex_lock(&conn_mutex);
        connections[slot].is_active = false;
        connections[slot].socket = -1;
        active_connections--;
        pthread_mutex_unlock(&conn_mutex);
        close(sock);
        return -1;
    }

    // Copy thread for receiving message
    if (pthread_create(&connections[slot].thread, NULL, receive_message_handler, conn_copy) != 0) {
        print_error("Failed to create receive thread");
        free(conn_copy);
        pthread_mutex_lock(&conn_mutex);
        connections[slot].is_active = false;
        connections[slot].socket = -1;
        active_connections--;
        pthread_mutex_unlock(&conn_mutex);
        close(sock);
        return -1;
    }

    // Detach thread
    pthread_detach(connections[slot].thread);

    return 0;
 }

 int terminate_connection(int conn_id) {
    connection_t* conn = find_connection_by_id(conn_id);

    if (!conn) {
        print_error("Connection not found!");
        return -1;
    }

    // Send termination notification
    char msg[MAX_MESSAGE_LENGTH];
    snprintf(msg, MAX_MESSAGE_LENGTH, "Connection terminated by peer");

    // Try to send
    if (conn->socket >= 0) {
        send(conn->socket, msg, strlen(msg), 0);
    }

    pthread_mutex_lock(&conn_mutex);
    // Mark as inactive and close socket
    if (conn->is_active) {
        conn->is_active = false;

        if (conn->socket >= 0) {
            close(conn->socket);
            conn->socket = -1;
        }

        active_connections--;
    }
    pthread_mutex_unlock(&conn_mutex);

    return 0;
 }
 
 void list_connections(void) {
    pthread_mutex_lock(&conn_mutex);
    
    printf("\n-------- Connection List --------\n");
    printf("ID  |  IP Address        |  Port  |  Type\n");
    printf("----------------------------------------\n");
    
    int count = 0;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connections[i].is_active) {
            printf("%-4d|  %-18s|  %-6d|  %s\n", 
                   connections[i].id, 
                   connections[i].ip, 
                   connections[i].port,
                   connections[i].is_incoming ? "Incoming" : "Outgoing");
            count++;
        }
    }
    
    if (count == 0) {
        printf("No active connections\n");
    }
    
    printf("----------------------------------------\n");
    printf("Total: %d connection(s)\n", count);
    
    pthread_mutex_unlock(&conn_mutex);
 }

 connection_t* find_connection_by_id(int conn_id) {
    pthread_mutex_lock(&conn_mutex);

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connections[i].is_active && connections[i].id == conn_id) {
            pthread_mutex_unlock(&conn_mutex);
            return &connections[i];
        }
    }

    pthread_mutex_unlock(&conn_mutex);
    return NULL;
 }

 void close_all_connections(void) {
    // Stop accepting new connections
    if (server_socket >= 0) {
        close(server_socket);
        server_socket = -1;
    }

    // Close all active connections
    pthread_mutex_lock(&conn_mutex);

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connections[i].is_active && connections[i].socket >= 0) {
            close(connections[i].socket);
            connections[i].socket = -1;
            connections[i].is_active = false;
        }
    }

    active_connections = 0;
    pthread_mutex_unlock(&conn_mutex);
    
    // Destroy mutex
    pthread_mutex_destroy(&conn_mutex);
 }

 static int find_free_slot(void) {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (!connections[i].is_active) {
            return i;
        }
    }
    return -1;
 }

 static int check_duplicate_connection(const char* ip, int port) {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connections[i].is_active && strcmp(connections[i].ip, ip) == 0
            && connections[i].port == port) {
            return i;
        }
    }
    return -1;
 }