/**
 * @file main.c
 * @brief Program demonstrating POSIX message queue for IPC between parent and child processes
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <mqueue.h>
 #include <sys/wait.h>
 #include <errno.h>
 
 // Constants for message queue configuration
 #define QUEUE_NAME      "/mqueue"
 #define MAX_MSG_SIZE    256
 #define MAX_MESSAGES    10
 #define MSG_PRIORITY    1
 
 int main(int argc, char const *argv[])
 {
     mqd_t mq;                     // Message queue descriptor
     struct mq_attr attr;          // Message queue attributes
     pid_t child_pid;              // Process ID for child
     char buffer[MAX_MSG_SIZE];    // Buffer for message data
     int status;                   // Status from waitpid
 
     // Set message queue attributes
     attr.mq_flags = 0;                  // Blocking mode
     attr.mq_maxmsg = MAX_MESSAGES;      // Maximum number of messages in queue
     attr.mq_msgsize = MAX_MSG_SIZE;     // Maximum size of a message in bytes
     attr.mq_curmsgs = 0;                // Current number of messages in queue (ignored for mq_open)
 
     // Create the message queue with read/write access
     mq = mq_open(QUEUE_NAME, O_CREAT | O_EXCL | O_RDWR, 0644, &attr);   
     if (mq == (mqd_t)(-1)) {
         // If queue already exists, try to open it without the O_EXCL flag
         if (errno == EEXIST) {
             mq = mq_open(QUEUE_NAME, O_RDWR);
             if (mq == (mqd_t)(-1)) {
                 perror("Error: Failed to open existing message queue");
                 exit(EXIT_FAILURE);
             }
         } else {
             perror("Error: Failed to create message queue");
             exit(EXIT_FAILURE);
         }
     }
 
     // Create a child process using fork()
     child_pid = fork();
 
     if (child_pid < 0) {
         // Fork failed - clean up and exit
         perror("Error: Fork failed");
         mq_close(mq);
         mq_unlink(QUEUE_NAME);
         exit(EXIT_FAILURE);
     }
     // Child process
     else if (child_pid == 0) {
         printf("Child process ID: %d\n", getpid());
 
         // Close the message queue descriptor inherited from parent
         if (mq_close(mq) == -1) {
             perror("Error: [Child] Failed to close queue descriptor");
         }
 
         // Open the queue for reading only
         mq = mq_open(QUEUE_NAME, O_RDONLY);
         if (mq == (mqd_t)(-1)) {
             perror("Error: [Child] Failed to open message queue for reading");
             exit(EXIT_FAILURE);
         }
 
         // Initialize buffer to zeros to ensure proper string termination
         memset(buffer, 0, MAX_MSG_SIZE);
 
         /* 
          * Receive message:
          * - buffer: Where to store the message
          * - MAX_MSG_SIZE: Maximum number of bytes to receive
          * - NULL: Don't store priority value
          */
         ssize_t bytes_received = mq_receive(mq, buffer, MAX_MSG_SIZE, NULL);
         if (bytes_received == -1) {
             perror("Error: [Child] Failed to receive message");
             mq_close(mq);
             exit(EXIT_FAILURE);
         }
         
         // Print the received message
         printf("[Child] Message received: %s\n", buffer);
         
         // Close the queue in the child process
         if (mq_close(mq) == -1) {
             perror("Error: [Child] Failed to close queue");
         }
         exit(EXIT_SUCCESS);
     }
     // Parent process
     else {
         printf("Parent process ID: %d\n", getpid());
 
         // Message to send to child process
         char *message = "Hello TungNHS";
 
         // Small delay to ensure child process is ready to receive
         // In production code, proper synchronization would be better
         sleep(1);
 
         /* 
          * Send message:
          * - message: The message to send
          * - strlen(message) + 1: Length of message (including null terminator)
          * - MSG_PRIORITY: Priority of this message
          */
         printf("[Parent] Sending message: %s\n", message);
         if (mq_send(mq, message, strlen(message) + 1, MSG_PRIORITY) == -1) {
             perror("Error: [Parent] Failed to send message");
             mq_close(mq);
             mq_unlink(QUEUE_NAME);
             exit(EXIT_FAILURE);
         }
 
         // Wait for child process to complete
         if (waitpid(child_pid, &status, 0) == -1) {
             perror("Error: waitpid failed");
         } else if (WIFEXITED(status)) {
             printf("[Parent] Child exited with status %d\n", WEXITSTATUS(status));
         }
         
         // Clean up by closing and removing the message queue
         printf("[Parent] Cleaning up message queue...\n");
         
         if (mq_close(mq) == -1) {
             perror("Error: [Parent] Failed to close queue");
         }
         
         if (mq_unlink(QUEUE_NAME) == -1) {
             perror("Error: [Parent] Failed to unlink queue");
         }
 
         return EXIT_SUCCESS;
     }
 }