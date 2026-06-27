/**
 * @file main.c
 * @brief Program demonstrating exec function to replace a process image
 */

 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <sys/wait.h> 
 
 int main(int argc, char const *argv[])
 {
     pid_t child_pid; // Define a variable to store process ID
 
     // Check if the number of command-line arguments is exactly 2
     if (argc != 2) {
         fprintf(stderr, "Usage: %s <option>\n", argv[0]);
         fprintf(stderr, "Please provide exactly 1 argument\n");
         return -1;
     }
     
     // Create a child process using fork()
     child_pid = fork();
 
     if (child_pid == 0) {
         // Code executed by child process
         printf("Child process running:\n");
         
         // Check the argument passed to the program and execute the appropriate command
         if (strcmp(argv[1], "1") == 0) {
             printf("Executing 'ls' command:\n");
             // Use execlp to replace the child process with the "ls -l" command
             execlp("ls", "ls", "-l", NULL);
         }
         else if (strcmp(argv[1], "2") == 0) {
             printf("Executing 'date' command:\n");
             // Use execlp to replace the child process with the "date" command
             execlp("date", "date", NULL);
         }
         else {
             fprintf(stderr, "Invalid command number: %s, choose (1 or 2)\n", argv[1]);
             exit(1);
         }
 
         // This code will only execute if execlp fails
         fprintf(stderr, "Error: execlp failed\n");
         exit(1);
     }
     else if (child_pid > 0) {
         // Code executed by parent process
         printf("Parent process running:\n");
         wait(NULL); // Wait for the child process to finish
         printf("Child process completed\n");
     }
     else {
         // Fork failed
         fprintf(stderr, "Error: Fork failed\n");
         return -1;
     }
 
     return 0;
 }