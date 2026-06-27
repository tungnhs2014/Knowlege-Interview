/**
 * main.c - Entry point for the Chat Application
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <signal.h>
 #include <unistd.h>
 #include "command.h"
 #include "connection.h"
 #include "message.h"
 #include "utils.h"

 #define MAX_COMAND_LENGTH 256

 int main(int argc, char const *argv[])
 {
    // Validate command line arguments
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Parase and validate port number
    int port = atoi(argv[1]);
    if (port <= 0 || port >= 65535) {
        print_error("Invalid port number. Port must be between 1 and 65535");
        return EXIT_FAILURE;
    }

    // Register signal handler for clean termination
    if (signal(SIGINT, handle_signal) == SIG_ERR) {
        print_error("Cannot handle SINGINT");
        return EXIT_FAILURE;
    }

    // Initialize sever socket
    if (initialize_server(port) != 0) {
        print_error("Failed to initialize sever socket");
        return EXIT_FAILURE;
    }

    // Start connection listener thread
    if (start_connection_listener() != 0) {
        print_error("Failed to start connection listener");
        cleanup_resources();
        return EXIT_FAILURE;
    }

    // Display welcome message and command help
    printf("Chat Application started on port: %d\n", port);
    display_help();

    // Main command processing loop
    char command_line[MAX_COMAND_LENGTH];
    while (1) {
        // Display prompt and get user input
        printf("Enter command: ");
        if (fgets(command_line, sizeof(command_line), stdin) == NULL) {
            // Handle EOF or read error
            break;
        }

        // Process the command
        process_command(command_line);
    }   

    // Clean up resources before exiting
    cleanup_resources();
    return EXIT_SUCCESS;
 }


 
 