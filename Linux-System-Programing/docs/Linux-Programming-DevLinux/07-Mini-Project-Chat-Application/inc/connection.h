/**
 * connection.h - Connection management for the chat application
 * 
 * Manages TCP connections between peers, handles connection creation,
 * termination, and maintains the connection list.
 */

 #ifndef CONNECTION_H
 #define CONNECTION_H
 
 #include <stdbool.h>
 #include <netinet/in.h>
 #include <pthread.h>
 
 // Maximum number of connections the application can handle
 #define MAX_CONNECTIONS 100
 
 // Maximum length of IP address string
 #define IP_LENGTH 16
 
 /**
  * Structure to represent a connection to another peer
  */
 typedef struct {
     int id;                     // Connection ID (for user commands)
     int socket;                 // Socket file descriptor
     char ip[IP_LENGTH];         // Peer IP address
     int port;                   // Peer port number
     struct sockaddr_in addr;    // Socket address structure
     pthread_t thread;           // Thread for receiving messages
     bool is_active;             // Whether connection is active
     bool is_incoming;           // Whether connection was initiated by peer
 } connection_t;
 
 /**
  * Initialize the server socket for the local device
  * 
  * @param port Port number to listen on
  * @return 0 on success, -1 on failure
  */
 int initialize_server(int port);
 
 /**
  * Get the listening port of this application
  * 
  * @return Port number
  */
 int get_listening_port(void);
 
 /**
  * Create a new connection to a peer
  * 
  * @param ip IP address of the peer
  * @param port Port number of the peer
  * @return 0 on success, -1 on failure
  */
 int connect_to_peer(const char *ip, int port);
 
 /**
  * Terminate a connection by its ID
  * 
  * @param conn_id Connection ID
  * @return 0 on success, -1 on failure
  */
 int terminate_connection(int conn_id);
 
 /**
  * Start the connection listener thread
  * 
  * @return 0 on success, -1 on failure
  */
 int start_connection_listener(void);
 
 /**
  * Display the list of all active connections
  */
 void list_connections(void);
 
 /**
  * Find a connection by its ID
  * 
  * @param conn_id Connection ID
  * @return Pointer to the connection, NULL if not found
  */
 connection_t* find_connection_by_id(int conn_id);
 
 /**
  * Close all connections and free resources
  */
 void close_all_connections(void);
 
 /**
  * Get server socket
  * 
  * @return Server socket file descriptor
  */
 int get_server_socket(void);
 
 #endif /* CONNECTION_H */