/**
 * message.c - Message handling implementation
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <errno.h>
 #include "message.h"
 #include "connection.h"
 #include "utils.h"

 int send_message(int conn_id, const char *message) {
    // Check message length
    if (strlen(message) > MAX_MESSAGE_LENGTH -1) {
        print_error("Message too long. Maxium length is 100 characters");
        return -1;
    }

    // Find the connetion
    connection_t* conn = find_connection_by_id(conn_id);
    if (!conn) {
        print_error("Connection is not active");
        return -1;
    }

    // Send the message
    ssize_t bytes_sent = send(conn->socket, message, strlen(message), 0);
    if (bytes_sent < 0) {
        print_error("Failed to send message");
        return -1;
    }

    return 0;
 }

 void process_received_message(const char *message, const char *sender_ip, int sender_port) {
    if (!message || !sender_ip) {
        return;
    } 
    
    // Display the message with sender information
    printf("\n***Message received from: %s\n", sender_ip);
    printf("***Sender Port:          %d\n", sender_port);
    printf("-->Message:              %s\n", message);
    
    // Restore the command prompt
    printf("\nEnter command: ");
    fflush(stdout);
 }

 void* receive_message_handler(void *arg) {
    connection_t *conn = (connection_t *)arg;
    if (!conn) {
        return NULL;
    }

    /**
     * Create local copies of connecion information
     * to avoid race conditions if the connection is modified
     */
    int socket = conn->socket;
    char ip[IP_LENGTH];
    int port = conn->port;
    int id = conn->id;
    strncpy(ip, conn->ip, IP_LENGTH -1);
    ip[IP_LENGTH -1] = '\0';

    // Free the malloc'd connection copy
    free(conn);
    
    // Buffer for receiving messages
    char buffer[MAX_MESSAGE_LENGTH];

    // Receive message in a loop
    while (1) {
        memset(buffer, 0, sizeof(buffer));

        // Receive message 
        ssize_t byte_recv = recv(socket, buffer, MAX_MESSAGE_LENGTH - 1, 0);

        // Check for errors or connection closed
        if (byte_recv <= 0) {
            // Check if interrupted by signal
            if (byte_recv < 0 && errno == EINTR) {
                continue;
            }

            // Mark connection as inactive
            connection_t *inactive_conn = find_connection_by_id(id);
            if (inactive_conn) {
                inactive_conn->is_active = false;
                inactive_conn->socket = -1;
            }

            printf("\nConnection with %s:%d closed\n", ip, port);
            printf("Enter command: ");
            fflush(stdout);

            // Close socket and exit thread
            close(socket);
            break;
        }

        // Ensure null-termination
        buffer[byte_recv] = '\0';

        // Process the received message;
        process_received_message(buffer, ip, port);
    }
    
    return NULL;
 }
