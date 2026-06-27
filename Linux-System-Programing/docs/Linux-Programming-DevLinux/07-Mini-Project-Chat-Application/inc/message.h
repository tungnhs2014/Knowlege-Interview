/**
 * message.h - Message handling for the chat application
 * 
 * Defines structures and functions for sending and receiving messages
 * between peers.
 */

 #ifndef MESSAGE_H
 #define MESSAGE_H
 
 #include "connection.h"
 
 // Maximum message length (including null terminator)
 #define MAX_MESSAGE_LENGTH 101
 
 /**
  * Message structure
  */
 typedef struct {
     char content[MAX_MESSAGE_LENGTH];  // Message content
     char sender_ip[IP_LENGTH];         // IP address of sender
     int sender_port;                   // Port of sender
 } message_t;
 
 /**
  * Send a message to a peer
  * 
  * @param conn_id Connection ID
  * @param message Message to send
  * @return 0 on success, -1 on failure
  */
 int send_message(int conn_id, const char *message);
 
 /**
  * Thread handler function for receiving messages
  * 
  * @param arg Pointer to the connection_t structure
  * @return NULL when thread exits
  */
 void* receive_message_handler(void *arg);
 
 /**
  * Format a message with sender information
  * 
  * @param msg Pointer to message structure
  * @param content Message content
  * @param sender_ip Sender's IP address
  * @param sender_port Sender's port
  * @return 0 on success, -1 on failure
  */
 int format_message(message_t *msg, const char *content, const char *sender_ip, int sender_port);
 
 /**
  * Process a received message
  * 
  * @param message The received message
  * @param sender_ip Sender's IP address
  * @param sender_port Sender's port
  */
 void process_received_message(const char *message, const char *sender_ip, int sender_port);
 
 #endif /* MESSAGE_H */