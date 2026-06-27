/**
 * @file main.c
 * @brief Program demonstrating communication between three processes using POSIX message queues
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <mqueue.h>
 #include <sys/wait.h>
 #include <ctype.h>
 #include <errno.h>
 
 // Constants for message queue configuration
 #define QUEUE_NAME      "/share_queue" 
 #define MAX_MSG_SIZE    256 
 #define MAX_MESSAGES    10
 #define MSG_PRIORITY    1
 
 /**
  * Safely closes a message queue and handles errors
  * @param mqd Message queue descriptor
  */
 void close_queue(mqd_t mqd) {
     if (mq_close(mqd) == -1) {
         fprintf(stderr, "Error: Failed to close queue: %s\n", strerror(errno));
     }
 }
 
 /**
  * Safely unlinks a message queue and handles errors
  * @param name Name of the queue
  */
 void unlink_queue(const char *name) {
     if (mq_unlink(name) == -1) {
         fprintf(stderr, "Error: Failed to unlink queue: %s\n", strerror(errno));
     }
 }
 
 /**
  * Opens a message queue with specified flags and handles errors
  * @param name Queue name
  * @param flags Open flags
  * @param attr Queue attributes (NULL for default)
  * @return Message queue descriptor or exits on failure
  */
 mqd_t open_queue(const char *name, int flags, struct mq_attr *attr) {
     mqd_t mqd;
     
     mqd = mq_open(name, flags, 0644, attr);
     if (mqd == (mqd_t)(-1)) {
         fprintf(stderr, "Error: Failed to open queue %s: %s\n", name, strerror(errno));
         exit(EXIT_FAILURE);
     }
     
     return mqd;
 }
 
 /**
  * Sends a message to a queue and handles errors
  * @param mqd Message queue descriptor
  * @param message Message to send
  * @param priority Message priority
  */
 void send_message(mqd_t mqd, const char *message, unsigned int priority) {
     if (mq_send(mqd, message, strlen(message) + 1, priority) == -1) {
         fprintf(stderr, "Error: Failed to send message: %s\n", strerror(errno));
         close_queue(mqd);
         exit(EXIT_FAILURE);
     }
 }
 
 /**
  * Receives a message from a queue and handles errors
  * @param mqd Message queue descriptor
  * @param buffer Buffer to store the message
  * @param size Buffer size
  * @return Number of bytes received
  */
 ssize_t receive_message(mqd_t mqd, char *buffer, size_t size) {
     ssize_t bytes;
     
     bytes = mq_receive(mqd, buffer, size, NULL);
     if (bytes == -1) {
         fprintf(stderr, "Error: Failed to receive message: %s\n", strerror(errno));
         close_queue(mqd);
         exit(EXIT_FAILURE);
     }
     
     return bytes;
 }
 
 /**
  * Convert a string to uppercase in-place
  * @param str String to convert
  */
 void to_uppercase(char *str) {
     for (int i = 0; str[i]; i++) {
         str[i] = toupper(str[i]);
     }
 }
 
 int main() {
     mqd_t mq;
     pid_t pid1, pid2;
     char msg[MAX_MSG_SIZE];
     int status;
 
     // Set up the message queue attributes
     struct mq_attr attr;
     attr.mq_flags = 0;                  // Blocking mode
     attr.mq_maxmsg = MAX_MESSAGES;      // Maximum number of messages
     attr.mq_msgsize = MAX_MSG_SIZE;     // Maximum size of each message
     attr.mq_curmsgs = 0;                // Current number of messages (ignored for mq_open)
 
     // Create the message queue
     mq = mq_open(QUEUE_NAME, O_CREAT | O_EXCL | O_RDWR, 0644, &attr);
     if (mq == (mqd_t)(-1)) {
         // If queue already exists, open it without O_EXCL
         if (errno == EEXIST) {
             mq = open_queue(QUEUE_NAME, O_RDWR, NULL);
         } else {
             perror("Error: Failed to create message queue");
             exit(EXIT_FAILURE);
         }
     }
 
     printf("[Parent] Process ID: %d\n", getpid());
 
     // Create the first child process (child 1)
     pid1 = fork();
 
     if (pid1 < 0) {
         // If fork fails for child 1, print error and exit
         perror("Error: Fork failed (child 1)");
         close_queue(mq);
         unlink_queue(QUEUE_NAME);
         exit(EXIT_FAILURE);
     } 
     else if (pid1 == 0) {
         // Child 1: Receive message, convert to uppercase, and send to child 2
         printf("[Child 1] Process ID: %d\n", getpid());
 
         // Close inherited queue descriptor and reopen
         close_queue(mq);
         mq = open_queue(QUEUE_NAME, O_RDWR, NULL);
 
         // Receive the original message from the parent
         receive_message(mq, msg, MAX_MSG_SIZE);
 
         // Convert the message to uppercase
         to_uppercase(msg);
 
         // Send the uppercase message to child 2
         sleep(1);  // Ensure child 2 is ready before sending
         send_message(mq, msg, MSG_PRIORITY);
 
         close_queue(mq);
         exit(EXIT_SUCCESS);
     } 
     else {
         // Create the second child process (child 2)
         pid2 = fork();
 
         if (pid2 < 0) {
             // If fork fails for child 2, print error and exit
             perror("Error: Fork failed (child 2)");
             close_queue(mq);
             unlink_queue(QUEUE_NAME);
             waitpid(pid1, NULL, 0);  // Wait for child 1 before exiting
             exit(EXIT_FAILURE);
         }
         else if (pid2 == 0) {
             // Child 2: Receive the uppercase message and print it
             printf("[Child 2] Process ID: %d\n", getpid());
 
             // Close inherited queue descriptor and reopen
             close_queue(mq);
             mq = open_queue(QUEUE_NAME, O_RDONLY, NULL);
 
             // Wait for the uppercase message from child 1
             receive_message(mq, msg, MAX_MSG_SIZE);
 
             // Print the received uppercase message
             printf("Child 2 received message: %s\n", msg);
 
             close_queue(mq);
             exit(EXIT_SUCCESS);
         }
         else {
             // Parent process: Prepare and send the original message
             const char *original_message = "Hello Tungnhs Nguyen huu son Tung!";
             strncpy(msg, original_message, MAX_MSG_SIZE - 1);
             msg[MAX_MSG_SIZE - 1] = '\0';  // Ensure null termination
 
             printf("[Parent] Sending message from PID: %d\n", getpid());
 
             sleep(1);  // Sleep to ensure children are ready before sending
 
             // Send the original message to child 1
             send_message(mq, msg, MSG_PRIORITY);
 
             // Wait for both children to finish
             if (waitpid(pid1, &status, 0) == -1) {
                 perror("Error: waitpid (child 1) failed");
             } else if (WIFEXITED(status)) {
                 printf("[Parent] Child 1 exited with status %d\n", WEXITSTATUS(status));
             }
             
             if (waitpid(pid2, &status, 0) == -1) {
                 perror("Error: waitpid (child 2) failed");
             } else if (WIFEXITED(status)) {
                 printf("[Parent] Child 2 exited with status %d\n", WEXITSTATUS(status));
             }
 
             // Clean up the message queue
             close_queue(mq);
             unlink_queue(QUEUE_NAME);
 
             printf("[Parent] Done.\n");
             return EXIT_SUCCESS;
         }
     }
 }