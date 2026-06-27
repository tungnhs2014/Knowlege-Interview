/**
 * command.h - Command processing for the chat application
 * 
 * Defines command types and functions for processing user input
 * in the chat application.
 */

 #ifndef COMMAND_H
 #define COMMAND_H
 
 /**
  * Command types
  */
 typedef enum {
     CMD_HELP,       // Display help information
     CMD_MYIP,       // Display my IP address
     CMD_MYPORT,     // Display my port
     CMD_CONNECT,    // Connect to another peer
     CMD_LIST,       // List connections
     CMD_TERMINATE,  // Terminate a connection
     CMD_SEND,       // Send a message
     CMD_EXIT,       // Exit the application
     CMD_UNKNOWN     // Unknown command
 } command_t;
 
 /**
  * Process a command string from the user
  * 
  * @param command_line Command string
  */
 void process_command(char *command_line);
 
 /**
  * Parse a command string to determine its type
  * 
  * @param cmd Command string
  * @return Command type
  */
 command_t parse_command(const char *cmd);
 
 /**
  * Display help information for all available commands
  */
 void display_help(void);
 
 #endif /* COMMAND_H */