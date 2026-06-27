/**
 * @file main.c
 * @brief Program for process creation and monitoring using fork()
 */

 #include <stdio.h>
 #include <unistd.h>
 #include <sys/wait.h> 
 
 int main(int argc, char const *argv[])
 {
     // Define a variable to store process ID
     pid_t child_pid; 
 
     /**
      * Create a child process using fork()
      * fork() returns: 
      * - Child PID to the parent
      * - 0 to the child
      * - -1 if error*/
     child_pid = fork();
 
     if (child_pid == 0) {
         // This code runs in the child process
         printf("Child process:\n");
         printf("PID of child process: %d\n", getpid());
         printf("PID of parent process: %d\n", getppid());
 
         // Sleep for 5 seconds to simulate work in the child process
         sleep(5);
     }
     else if (child_pid > 0) {
         // This code runs in the parent process
         printf("Parent process:\n");
         printf("PID of parent process: %d\n", getpid());
         printf("PID of child process: %d\n", child_pid);
 
         // Wait for the child process to complete
         wait(NULL);
         printf("Child process has completed\n");
     }
     else {
         // Fork failed
         fprintf(stderr, "Error: Fork failed\n");
         return -1;
     }
 
     return 0;
 }