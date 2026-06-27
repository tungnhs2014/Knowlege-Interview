/**
 * @file main.c
 * @brief Program demonstrating signal handling between parent and child processes
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <signal.h>
 #include <unistd.h>
 #include <sys/wait.h> 
 
 /**
  * Signal handler function for SIGUSR1
  * @param sig Signal number
  */
 void signal_handler(int sig);
 
 int main(int argc, char const *argv[])
 {
     pid_t child_pid; // Define a variable to store process ID
 
     // Create a child process using fork()
     child_pid = fork();
 
     if (child_pid < 0) {
         // Fork failed
         fprintf(stderr, "Error: Fork failed\n");
         exit(1);
     }
     else if (child_pid == 0) {
         // Child process code
         printf("Child process started: PID: %d\n", getpid());
 
         // Register the signal handler for SIGUSR1
         signal(SIGUSR1, signal_handler);
 
         // Keep the child process alive to wait for the signal
         sleep(10);
         
         // This code will only execute if the signal handler doesn't exit
         printf("Child process finished normally\n");
     }
     else {
         // Parent process code
         printf("Parent process started: PID: %d\n", getpid());
         
         // Sleep for 2 seconds before sending the signal
         sleep(2);
 
         // Send the SIGUSR1 signal to the child process
         kill(child_pid, SIGUSR1);
         printf("Parent sent SIGUSR1 to child process\n");
 
         // Wait for the child process to finish
         wait(NULL);
         printf("Parent process detected child termination\n");
     }
 
     return 0;
 }
 
 // Signal handler function for SIGUSR1
 void signal_handler(int sig) {
     printf("Child process received signal SIGUSR1\n");
     exit(0); // Exit with success code
 }