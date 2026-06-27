/**
 * @file main.c
 * @brief Program demonstrating pipe-based communication between parent and child processes
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <sys/types.h>
 #include <sys/wait.h>
 
 #define BUFFER_SIZE 100  // Size of buffer for reading from pipe
 
 int main(int argc, char const *argv[])
 {
     int pipe_fd[2];        // File descriptors for pipe [0] = read end, [1] = write end
     pid_t child_pid;       // Process ID of the child
     int status;            // For storing wait status
 
     char *message = "Hello from parent process";
     char buffer[BUFFER_SIZE];
 
     // Create pipe
     if (pipe(pipe_fd) == -1) {
         perror("Error: Failed to create pipe");
         exit(EXIT_FAILURE);
     }
 
     // Fork a new process
     child_pid = fork();
 
     if (child_pid < 0) { // Error handling for fork
         perror("Error: Fork failed");
         exit(EXIT_FAILURE);
     }
     // Parent process
     else if (child_pid > 0) {
         printf("Parent process (PID: %d) sending message: %s\n", getpid(), message);
 
         // Close read end as parent only writes
         close(pipe_fd[0]);
 
         // Write message to the pipe
         ssize_t bytes_written = write(pipe_fd[1], message, strlen(message) + 1); // +1 includes null terminator
         
         if (bytes_written == -1) {
             perror("Error: Failed to write to pipe");
             close(pipe_fd[1]);
             exit(EXIT_FAILURE);
         }
         
         // Close write end after writing is complete
         close(pipe_fd[1]);
 
         // Wait for child to finish
         if (waitpid(child_pid, &status, 0) == -1) {
             perror("Error: waitpid failed");
             exit(EXIT_FAILURE);
         }
         
         if (WIFEXITED(status)) {
             printf("Parent process: Child exited with status %d\n", WEXITSTATUS(status));
         }
         
         printf("Parent process terminated\n");
     }
     // Child process
     else {
         // Close write end as child only reads
         close(pipe_fd[1]);
 
         // Read data from the pipe
         ssize_t bytes_read = read(pipe_fd[0], buffer, sizeof(buffer) - 1); // -1 to ensure space for null terminator
 
         if (bytes_read > 0) {
             // Ensure the string is null-terminated
             buffer[bytes_read] = '\0';
             printf("Child process (PID: %d) received message: %s\n", getpid(), buffer);
         }
         else if (bytes_read == 0) {
             printf("Child process: Pipe was closed before any data was available\n");
         }
         else {
             perror("Error: Failed to read from pipe");
             close(pipe_fd[0]);
             exit(EXIT_FAILURE);
         }
 
         // Close read end after reading is complete
         close(pipe_fd[0]);
 
         printf("Child process terminated\n");
         exit(EXIT_SUCCESS); // Explicitly exit with success status
     }
 
     return 0;
 }