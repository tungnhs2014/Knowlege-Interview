/**
 * @file main.c
 * @brief Program demonstrating communication among three processes using two pipes
 */

 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <sys/wait.h>
 
 #define BUFFER_SIZE_1 100  // Size of buffer for first pipe
 #define BUFFER_SIZE_2 200  // Size of buffer for second pipe (larger to accommodate modified message)
 
 int main(int argc, char const *argv[])
 {
     int pipe1[2]; // Pipe for parent to child1 communication
     int pipe2[2]; // Pipe for child1 to child2 communication
     pid_t child1_pid, child2_pid;
     int status;
 
     char *message = "Hello TungNHS";
     char buffer1[BUFFER_SIZE_1];
     char buffer2[BUFFER_SIZE_2];
 
     // Create the first pipe (parent => child1)
     if (pipe(pipe1) == -1) {
         perror("Error: Failed to create first pipe");
         exit(EXIT_FAILURE);
     }
     
     // Create the second pipe (child1 => child2)
     if (pipe(pipe2) == -1) {
         perror("Error: Failed to create second pipe");
         close(pipe1[0]);
         close(pipe1[1]);
         exit(EXIT_FAILURE);
     }
 
     // Fork first child process
     child1_pid = fork();
     
     if (child1_pid < 0) {
         perror("Error: First fork failed");
         // Clean up pipes
         close(pipe1[0]); close(pipe1[1]);
         close(pipe2[0]); close(pipe2[1]);
         exit(EXIT_FAILURE);
     }
     // Parent process
     else if (child1_pid > 0) {
         // Fork second child process
         child2_pid = fork();
 
         if (child2_pid < 0) {
             perror("Error: Second fork failed");
             // Clean up pipes
             close(pipe1[0]); close(pipe1[1]);
             close(pipe2[0]); close(pipe2[1]);
             exit(EXIT_FAILURE);
         }
         else if (child2_pid > 0) { // Still in parent process
             printf("Parent process (PID: %d) sending a message: %s\n", getpid(), message);
 
             // Close unused pipe ends in parent
             close(pipe1[0]); // Close read end of pipe1
             close(pipe2[0]); // Close read end of pipe2
             close(pipe2[1]); // Close write end of pipe2
 
             // Write message to first child through pipe1
             ssize_t bytes_written = write(pipe1[1], message, strlen(message) + 1);
             if (bytes_written == -1) {
                 perror("Error: Failed to write to pipe1");
                 close(pipe1[1]);
                 exit(EXIT_FAILURE);
             }
             
             printf("Parent sent message to Child 1\n");
 
             // Close write end of pipe1 after using
             close(pipe1[1]);
             
             // Wait for both children to finish 
             waitpid(child1_pid, &status, 0);
             if (WIFEXITED(status)) {
                 printf("Parent: Child 1 exited with status %d\n", WEXITSTATUS(status));
             }
             
             waitpid(child2_pid, &status, 0);
             if (WIFEXITED(status)) {
                 printf("Parent: Child 2 exited with status %d\n", WEXITSTATUS(status));
             }
 
             printf("Parent process terminated\n");
         }
         // Child 2 process
         else {
             // Close unused pipe ends in child2
             close(pipe1[0]); // Close read end of pipe1
             close(pipe1[1]); // Close write end of pipe1
             close(pipe2[1]); // Close write end of pipe2
 
             // Read data from Child 1 through pipe2
             ssize_t bytes_read = read(pipe2[0], buffer2, sizeof(buffer2) - 1);
             
             if (bytes_read > 0) {
                 // Ensure null termination
                 buffer2[bytes_read] = '\0';
                 printf("Child 2 process (PID: %d) received message: %s\n", getpid(), buffer2);
             }
             else if (bytes_read == 0) {
                 printf("Child 2: Pipe was closed before any data was available\n");
             }
             else {
                 perror("Error: Child 2 failed to read data");
                 close(pipe2[0]);
                 exit(EXIT_FAILURE);
             }
             
             // Close read end of pipe2 after use
             close(pipe2[0]);
 
             printf("Child 2 process terminated\n");
             exit(EXIT_SUCCESS);
         }
     }
     // Child 1 process
     else {  
         // Close unused pipe ends in Child 1
         close(pipe1[1]); // Close write end of pipe1
         close(pipe2[0]); // Close read end of pipe2
 
         // Read data from parent through pipe1
         ssize_t bytes_read = read(pipe1[0], buffer1, sizeof(buffer1) - 1);
         
         if (bytes_read > 0) {
             // Ensure null termination
             buffer1[bytes_read] = '\0';
             printf("Child 1 process (PID: %d) received message: %s\n", getpid(), buffer1);
 
             // Modify the message
             if (snprintf(buffer2, sizeof(buffer2), "%s - Nguyen Huu Son Tung", buffer1) < 0) {
                 perror("Error: Failed to format message");
                 close(pipe1[0]);
                 close(pipe2[1]);
                 exit(EXIT_FAILURE);
             }
             
             printf("Child 1 modified message: %s\n", buffer2);
 
             // Send modified message to Child 2 through pipe2
             ssize_t bytes_written = write(pipe2[1], buffer2, strlen(buffer2) + 1);
             if (bytes_written == -1) {
                 perror("Error: Failed to write to pipe2");
                 close(pipe1[0]);
                 close(pipe2[1]);
                 exit(EXIT_FAILURE);
             }
             
             printf("Child 1 sent message to Child 2\n");
         }
         else if (bytes_read == 0) {
             printf("Child 1: Pipe was closed before any data was available\n");
         }
         else {
             perror("Error: Child 1 failed to read data");
             close(pipe1[0]);
             close(pipe2[1]);
             exit(EXIT_FAILURE);
         }  
 
         // Close used pipe ends after use
         close(pipe2[1]);
         close(pipe1[0]);
 
         printf("Child 1 process terminated\n");
         exit(EXIT_SUCCESS);
     }
 
     return 0;
 }