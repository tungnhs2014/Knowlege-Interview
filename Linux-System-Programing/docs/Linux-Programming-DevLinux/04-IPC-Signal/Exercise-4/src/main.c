/**
 * @file main.c
 * @brief Program demonstrating how to ignore SIGTSTP signal (Ctrl+Z)
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <signal.h>
 #include <unistd.h>
 
 /**
  * Signal handler function for SIGTSTP (Ctrl+Z)
  * @param sig Signal number
  */
 void sigtstp_handler(int sig);
 
 int main(int argc, char const *argv[]) {
     // Register the signal handler for SIGTSTP using the signal() function
     if (signal(SIGTSTP, sigtstp_handler) == SIG_ERR) {
         perror("Error: Failed to register signal handler");
         exit(EXIT_FAILURE);
     }
 
     printf("Program started. Press Ctrl + Z to test SIGTSTP handling\n");
 
     int count = 0;  // Counter to keep track of iterations
 
     // Main program loop - runs indefinitely until terminated by user
     while (1) {
         printf("Programming is running count: %d\r", count++);
         fflush(stdout);  // Force output to be displayed immediately
         sleep(1);
     }
     
     return 0;
 }
 
 /**
  * Signal handler function to handle SIGTSTP (Ctrl + Z)
  * Instead of suspending the program, it prints a message and continues execution
  */
 void sigtstp_handler(int sig) {
     printf("\nSIGTSTP ignored\n");
     fflush(stdout); // Ensure the message is displayed immediately
     
     // Re-register the signal handler (some systems reset it after each signal)
     signal(SIGTSTP, sigtstp_handler);
 }