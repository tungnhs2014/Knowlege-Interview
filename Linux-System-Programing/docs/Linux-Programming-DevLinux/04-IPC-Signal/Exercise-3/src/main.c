/**
 * @file main.c
 * @brief Program demonstrating inter-process communication using signals
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <signal.h>
 #include <sys/types.h>
 #include <sys/wait.h>
 
 // Global variable to count the number of signals received in child process
 volatile int signal_count = 0;
 
 /**
  * Signal handler function for SIGUSR1
  * @param sig Signal number
  */
 void signal_handler(int sig);
 
 int main(int argc, char const *argv[]) {
     pid_t child_pid;
 
     // Create a new child process using fork()
     child_pid = fork();
 
     // Check if fork() failed
     if (child_pid < 0) {
         perror("Error: Fork() failed");
         exit(EXIT_FAILURE); 
     }
     // Child process
     else if (child_pid == 0) {
         printf("Child process started. PID: %d\n", getpid());
 
         // Register a signal handler for SIGUSR1 in the child process
         if (signal(SIGUSR1, signal_handler) == SIG_ERR) {
             perror("Error: Failed to register signal handler in child");
             exit(EXIT_FAILURE);
         }
 
         // Loop to keep the child process running
         // The child will exit after receiving 5 signals (managed in the signal handler)
         while (1) {
             pause();  // Wait for signals
         }
 
         // This code is never reached
         exit(EXIT_SUCCESS); 
     }
     // Parent process
     else {
         printf("Parent process started. PID: %d\n", getpid());
         
         // Send 5 signals to the child process, every 2 seconds
         for (int i = 1; i <= 5; i++) {
             sleep(2);  // Sleep for 2 seconds before sending the next signal
             
             // Send SIGUSR1 signal to the child process
             if (kill(child_pid, SIGUSR1) < 0) {
                 perror("Error: Failed to send signal");
                 kill(child_pid, SIGTERM);  // Clean up
                 exit(EXIT_FAILURE);
             }
             
             printf("Parent sent SIGUSR1 to child process\n");
         }
         
         printf("Sent 5 signals. Terminating child process...\n");
 
         // Send SIGTERM to terminate the child process
         if (kill(child_pid, SIGTERM) < 0) {
             perror("Error: Failed to terminate child process");
         }
 
         // Wait for the child process to terminate
         if (wait(NULL) < 0) {
             perror("Error: Wait failed");
         }
 
         printf("Child process terminated. Parent exiting\n");
     }
 
     return 0;
 }
 
 /**
  * Signal handler function to handle the received signal in the child process
  */
 void signal_handler(int sig) {
     signal_count++;
     printf("Child process received signal SIGUSR1\n");
     
     // If we've received 5 signals, exit the child process
     if (signal_count >= 5) {
         exit(EXIT_SUCCESS);
     }
     
     // Re-register the signal handler (some systems reset it after each signal)
     signal(SIGUSR1, signal_handler);
 }