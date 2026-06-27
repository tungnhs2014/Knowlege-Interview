/**
 * utils.h - Utility functions for the chat application
 * 
 * Provides common utility functions used throughout the application.
 */

 #ifndef UTILS_H
 #define UTILS_H
 
 #include <stdbool.h>
 
 /**
  * Print an error message to stderr
  * 
  * @param message Error message
  */
 void print_error(const char *message);
 
 /**
  * Signal handler for SIGINT (Ctrl+C)
  * 
  * @param sig Signal number
  */
 void handle_signal(int sig);
 
 /**
  * Get the local IP address (not loopback)
  * 
  * @param buffer Buffer to store the IP address
  * @param buffer_size Size of the buffer
  * @return true if successful, false otherwise
  */
 bool get_local_ip(char *buffer, int buffer_size);
 
 /**
  * Check if an IP address is valid
  * 
  * @param ip IP address string
  * @return true if valid, false otherwise
  */
 bool is_valid_ip(const char *ip);
 
 /**
  * Clean up resources before exiting
  */
 void cleanup_resources(void);
 
 /**
  * Check if two IP addresses and ports are the same
  * 
  * @param ip1 First IP address
  * @param port1 First port
  * @param ip2 Second IP address
  * @param port2 Second port
  * @return true if they match, false otherwise
  */
 bool is_same_address(const char *ip1, int port1, const char *ip2, int port2);
 
 #endif /* UTILS_H */