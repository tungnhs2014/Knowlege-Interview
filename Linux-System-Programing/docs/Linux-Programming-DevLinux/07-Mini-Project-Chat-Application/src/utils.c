/**
 * utils.c - Utility functions implementation
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <arpa/inet.h>
 #include <netinet/in.h>
 #include <signal.h>
 #include "utils.h"
 #include "connection.h"

 void print_error(const char *message) {
    if (!message) {
        return;
    }

    fprintf(stderr, "ERROR: %s\n", message);
 }

 void handle_signal(int sig) {
    if (sig == SIGINT) {
        printf("\nReceived interrupt signal. Shutting down...\n");
        cleanup_resources();
        exit(EXIT_SUCCESS);
    }
 }

 bool get_local_ip(char *buffer, int buffer_size) {
    if (!buffer || buffer_size < IP_LENGTH) {
        return false;
    }

    // Use hostname command to get IP
    FILE *cmd = popen("hostname -I | awk '{print $1}'", "r");
    if (!cmd) {
        return false;
    }

    // Read result
    if (fgets(buffer, buffer_size, cmd) != NULL) {
        // Remove trailing newline if present
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
        
        pclose(cmd);
        return true;
    }

    pclose(cmd);

    // Fallback to loopback if hostname command fails
    strncpy(buffer, "127.0.0.1", buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
     
    return true;
 }

 bool is_valid_ip(const char *ip) { 
    struct sockaddr_in sa;
    return ip && inet_pton(AF_INET, ip, &sa.sin_addr) == 1;
 }

 void cleanup_resources(void) {
    printf("Cleanning up resources...\n");

    // Close all connections
    close_all_connections();

    printf("All resources cleaned up.\n");
 }

 bool is_same_address(const char *ip1, int port1, const char *ip2, int port2) {
    return (ip1 && ip2 && strcmp(ip1, ip2) == 0 && port1 == port2);
 }