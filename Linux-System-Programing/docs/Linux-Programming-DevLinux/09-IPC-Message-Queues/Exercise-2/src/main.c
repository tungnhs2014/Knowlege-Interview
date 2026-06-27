/**
 * @file main.c
 * @brief Program demonstrating bidirectional communication using POSIX message queues
 */

 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <mqueue.h>
 #include <sys/wait.h>
 #include <errno.h>
 
 // Constants for message queue configuration
 #define QUEUE_PARENT_TO_CHILD   "/parent_to_child"
 #define QUEUE_CHILD_TO_PARENT   "/child_to_parent"
 #define MAX_MSG_SIZE            256
 #define MAX_MESSAGES            10
 #define MSG_PRIORITY            1
 
 /**
  * Safely closes a message queue and handles errors
  * @param mqd Message queue descriptor
  * @param name Name of the queue (for error reporting)
  */
 void close_queue(mqd_t mqd, const char *name) {
     if (mq_close(mqd) == -1) {
         fprintf(stderr, "Error: Failed to close %s: %s\n", name, strerror(errno));
     }
 }
 
 /**
  * Safely unlinks a message queue and handles errors
  * @param name Name of the queue
  */
 void unlink_queue(const char *name) {
     if (mq_unlink(name) == -1) {
         fprintf(stderr, "Error: Failed to unlink %s: %s\n", name, strerror(errno));
     }
 }
 
 /**
  * Cleans up all message queues
  * @param mq_ptc Parent to child queue descriptor
  * @param mq_ctp Child to parent queue descriptor
  */
 void cleanup_queues(mqd_t mq_ptc, mqd_t mq_ctp) {
     close_queue(mq_ptc, QUEUE_PARENT_TO_CHILD);
     close_queue(mq_ctp, QUEUE_CHILD_TO_PARENT);
     unlink_queue(QUEUE_PARENT_TO_CHILD);
     unlink_queue(QUEUE_CHILD_TO_PARENT);
 }
 
 int main(int argc, char const *argv[])
 {
     mqd_t mq_parent_to_child, mq_child_to_parent;
     struct mq_attr attr;
     pid_t child_pid;
     char buffer[MAX_MSG_SIZE];
     int status;
 
     // Set message queue attributes
     attr.mq_flags = 0;                  // Blocking mode
     attr.mq_maxmsg = MAX_MESSAGES;      // Maximum number of messages in queue
     attr.mq_msgsize = MAX_MSG_SIZE;     // Maximum size of a message in bytes
     attr.mq_curmsgs = 0;                // Current number of messages in queue (ignored for mq_open)
 
     // Create the first message queue (parent to child)
     mq_parent_to_child = mq_open(QUEUE_PARENT_TO_CHILD, O_CREAT | O_EXCL | O_RDWR, 0644, &attr);
     if (mq_parent_to_child == (mqd_t)(-1)) {
         // If queue already exists, try opening without O_EXCL
         if (errno == EEXIST) {
             mq_parent_to_child = mq_open(QUEUE_PARENT_TO_CHILD, O_RDWR);
             if (mq_parent_to_child == (mqd_t)(-1)) {
                 perror("Error: Failed to open existing parent-to-child queue");
                 exit(EXIT_FAILURE);
             }
         } else {
             perror("Error: Failed to create parent-to-child queue");
             exit(EXIT_FAILURE);
         }
     }
 
     // Create the second message queue (child to parent)
     mq_child_to_parent = mq_open(QUEUE_CHILD_TO_PARENT, O_CREAT | O_EXCL | O_RDWR, 0644, &attr);
     if (mq_child_to_parent == (mqd_t)(-1)) {
         // If queue already exists, try opening without O_EXCL
         if (errno == EEXIST) {
             mq_child_to_parent = mq_open(QUEUE_CHILD_TO_PARENT, O_RDWR);
             if (mq_child_to_parent == (mqd_t)(-1)) {
                 perror("Error: Failed to open existing child-to-parent queue");
                 close_queue(mq_parent_to_child, QUEUE_PARENT_TO_CHILD);
                 unlink_queue(QUEUE_PARENT_TO_CHILD);
                 exit(EXIT_FAILURE);
             }
         } else {
             perror("Error: Failed to create child-to-parent queue");
             close_queue(mq_parent_to_child, QUEUE_PARENT_TO_CHILD);
             unlink_queue(QUEUE_PARENT_TO_CHILD);
             exit(EXIT_FAILURE);
         }
     }
 
     // Create child process
     child_pid = fork();
 
     if (child_pid < 0) {
         // Fork failed - clean up and exit
         perror("Error: Fork failed");
         cleanup_queues(mq_parent_to_child, mq_child_to_parent);
         exit(EXIT_FAILURE);
     }
     // Child process
     else if (child_pid == 0) {
         printf("[Child] Process ID: %d\n", getpid());
 
         // Clear buffer before receiving message
         memset(buffer, 0, MAX_MSG_SIZE);
         
         // Receive message from parent
         ssize_t bytes_received = mq_receive(mq_parent_to_child, buffer, MAX_MSG_SIZE, NULL);
         if (bytes_received == -1) {
             perror("Error: [Child] Failed to receive message");
             close_queue(mq_parent_to_child, QUEUE_PARENT_TO_CHILD);
             close_queue(mq_child_to_parent, QUEUE_CHILD_TO_PARENT);
             exit(EXIT_FAILURE);
         }
         
         // Display the received message
         printf("[Child] Message received: %s\n", buffer);
 
         // Count characters in the received string
         int char_count = strlen(buffer);
         printf("[Child] Character count: %d\n", char_count);
 
         // Convert count to string for sending
         char count_str[20];  // Large enough for any integer
         snprintf(count_str, sizeof(count_str), "%d", char_count);
 
         // Sending character count back to parent process
         if (mq_send(mq_child_to_parent, count_str, strlen(count_str) + 1, MSG_PRIORITY) == -1) {
             perror("Error: [Child] Failed to send character count");
             close_queue(mq_parent_to_child, QUEUE_PARENT_TO_CHILD);
             close_queue(mq_child_to_parent, QUEUE_CHILD_TO_PARENT);
             exit(EXIT_FAILURE);
         }
 
         // Close message queues
         close_queue(mq_parent_to_child, QUEUE_PARENT_TO_CHILD);
         close_queue(mq_child_to_parent, QUEUE_CHILD_TO_PARENT);
         exit(EXIT_SUCCESS);
     } 
     // Parent process
     else {
         printf("[Parent] Process ID: %d\n", getpid());
         
         // Message to send to child process
         const char *message = "Hello, I am TungNHS";
 
         // Send message to child
         printf("[Parent] Sending message to child: %s\n", message);
         if (mq_send(mq_parent_to_child, message, strlen(message) + 1, MSG_PRIORITY) == -1) {
             perror("Error: [Parent] Failed to send message");
             cleanup_queues(mq_parent_to_child, mq_child_to_parent);
             exit(EXIT_FAILURE);
         }
         
         // Clear buffer before receiving message
         memset(buffer, 0, MAX_MSG_SIZE);
 
         // Receive character count from child
         ssize_t bytes_received = mq_receive(mq_child_to_parent, buffer, MAX_MSG_SIZE, NULL);
         if (bytes_received == -1) {
             perror("Error: [Parent] Failed to receive character count");
             cleanup_queues(mq_parent_to_child, mq_child_to_parent);
             exit(EXIT_FAILURE);
         }
 
         // Display character count received from child
         printf("[Parent] Character count received from child: %s\n", buffer);
 
         // Wait for child process to complete
         if (waitpid(child_pid, &status, 0) == -1) {
             perror("Error: waitpid failed");
         } else if (WIFEXITED(status)) {
             printf("[Parent] Child exited with status %d\n", WEXITSTATUS(status));
         }
 
         // Clean up message queues
         printf("[Parent] Cleaning up message queues...\n");
         cleanup_queues(mq_parent_to_child, mq_child_to_parent);
 
         return EXIT_SUCCESS;        
     }
 }