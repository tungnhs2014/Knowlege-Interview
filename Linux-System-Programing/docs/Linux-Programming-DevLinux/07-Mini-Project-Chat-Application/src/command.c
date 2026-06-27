/**
 * command.c - Command processing implementation
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include "command.h"
 #include "connection.h"
 #include "message.h"
 #include "utils.h"

 // Command strings matching the command_t enum
 static const char *COMMAND_STRINGS[] = {
    "help",
    "myip",
    "myport",
    "connect",
    "list",
    "terminate",
    "send",
    "exit"
 };

 command_t parse_command(const char *cmd) {
    if (!cmd) {
        return CMD_UNKNOWN;
    }

    // Compare with known commands
    for (int i = 0; i <= CMD_EXIT; i++) {
        if (strcmp(cmd, COMMAND_STRINGS[i]) == 0) {
            return (command_t)i;
        }
    }

    return CMD_UNKNOWN;
 }

 void display_help(void) {
    printf("\n-------- Command List --------\n");
    printf("help                         : Display all commands\n");
    printf("myip                         : Display your IP address\n");
    printf("myport                       : Display your port number\n");
    printf("connect <ip> <port>          : Connect to a peer\n");
    printf("list                         : List all active connections\n");
    printf("terminate <id>               : Terminate a connection\n");
    printf("send <id> <message>          : Send a message to a peer\n");
    printf("exit                         : Exit the application\n");
    printf("-----------------------------\n");
 }

 void process_command(char *command_line) {
    if (!command_line || strlen(command_line) == 0) {
        return;
    }

    // Remove tralling newline if present
    size_t len = strlen(command_line);
    if (command_line[len - 1] == '\n') {
        command_line[len - 1] = '\0';
    }

    // Extract command
    char command[20];
    if (sscanf(command_line, "%19s", command) != 1) {
        return;
    }

    // Parse command and execute
    switch (parse_command(command)) {
        case CMD_HELP:
            display_help();
            break;
        
        case CMD_MYIP: {
            char ip[IP_LENGTH];
            if (get_local_ip(ip, IP_LENGTH)) {
                printf("Your IP adderess: %s\n", ip);
            }
            else {
                print_error("Could not determine IP address");
            }
            break;
        }

        case CMD_MYPORT:
            printf("Your port: %d\n", get_listening_port());
            break;
        
        case CMD_CONNECT: {
            char ip[IP_LENGTH];
            int port;

            // Parse IP and port
            if (sscanf(command_line, "%*s %15s %d", ip, &port) != 2) {
                print_error("Invalid format. Usage: connect <ip> <port>");
                break;    
            }

            connect_to_peer(ip, port);
            break;
        }

        case CMD_LIST:
            list_connections();
            break;
        
        case CMD_TERMINATE: {
            int id;
            
            // Parse connection ID
            if (sscanf(command_line, "*s %d", &id) != 1) {
                print_error("Invalid format. Usage: terminate <id>");
                break;
            }
            
            if (terminate_connection(id) == 0) {
                printf("Connection %d terminated successfully.\n", id);
            }
            break;
        }

        case CMD_SEND: {
            int id;
            char message[MAX_MESSAGE_LENGTH];

            // Parse ID and extract message portion
            if (sscanf(command_line, "%*s %d %100[^\n]", &id, message) != 2) {
                print_error("Invalid format. Usage: send <id> <message>");
                break;
            }

            if (send_message(id, message) == 0) {
                printf("Message sent to connection %d.\n", id);
            }
            break;
        }

        case CMD_EXIT:
            printf("Exiting application...\n");
            cleanup_resources();
            exit(EXIT_SUCCESS);
            break;
        case CMD_UNKNOWN:
        default:
            printf("Unknown command. Type 'help' for available commands.\n");
            break;
    }

 }