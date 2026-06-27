/**
 * @file main.c
 * @brief Program that counts characters in a string using pipe communication
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <sys/wait.h>
 
 #define BUFFER_SIZE 256  // Size of buffer for reading from pipe
 
 int main(int argc, char const *argv[])
 {
     int pipe_fd[2];      // File descriptors for pipe [0] = read end, [1] = write end
     pid_t child_pid;     // Process ID of the child
     int status;          // For storing wait status
     
     char *message = "Welcome Nguyen Huu Son Tung";
     char buffer[BUFFER_SIZE];
 
     // Create pipe
     if (pipe(pipe_fd) == -1) {
         perror("Error: Failed to create pipe");
         exit(EXIT_FAILURE);
     }
 
     // Fork a new process
     child_pid = fork();
 
     if (child_pid < 0) {
         perror("Error: Fork failed");
         // Clean up pipe
         close(pipe_fd[0]);
         close(pipe_fd[1]);
         exit(EXIT_FAILURE);
     }
     // Parent process
     else if (child_pid > 0) {
         printf("Parent process (PID: %d) sending a message: %s\n", getpid(), message);
 
         // Close unused pipe end - read end
         close(pipe_fd[0]);
 
         // Write a message to pipe
         ssize_t bytes_written = write(pipe_fd[1], message, strlen(message) + 1);
         if (bytes_written == -1) {
             perror("Error: Failed to write to pipe");
             close(pipe_fd[1]);
             exit(EXIT_FAILURE);
         }
 
         // Close write end after using
         close(pipe_fd[1]);
 
         // Wait for child to finish
         if (waitpid(child_pid, &status, 0) == -1) {
             perror("Error: waitpid failed");
             exit(EXIT_FAILURE);
         }
         
         if (WIFEXITED(status)) {
             printf("Parent: Child exited with status %d\n", WEXITSTATUS(status));
         }
 
         printf("Parent process terminated\n");
     }
     // Child process
     else {
         // Close unused pipe end - write end
         close(pipe_fd[1]);
         
         // Read data from the pipe
         ssize_t bytes_read = read(pipe_fd[0], buffer, sizeof(buffer) - 1);
         
         if (bytes_read > 0) {
             // Ensure the string is null-terminated
             buffer[bytes_read] = '\0';
             
             // Count characters (excluding null terminator)
             int char_count = strlen(buffer);
             
             printf("Child process (PID: %d) received a message: %s\n", getpid(), buffer);
             printf("Child process counted %d characters in the received string\n", char_count);
         }
         else if (bytes_read == 0) {
             printf("Child process: Pipe was closed before any data was available\n");
         }
         else {
             perror("Error: Child process failed to read data");
             close(pipe_fd[0]);
             exit(EXIT_FAILURE);
         }
 
         // Close read end after using
         close(pipe_fd[0]);
         
         printf("Child process terminated\n");
         exit(EXIT_SUCCESS);
     }
 
     return 0;
 }