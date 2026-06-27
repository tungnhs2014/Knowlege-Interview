/**
 * @file main.c
 * @brief Program demonstrating signal handling with user input using select()
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <signal.h>
 #include <unistd.h>
 #include <sys/select.h>
 #include <errno.h>
 
 /**
  * Signal handler function for SIGINT (Ctrl + C)
  * @param sig Signal number
  */
 void sigint_handler(int sig);
 
 /**
  * Signal handler function for SIGTERM (termination signal)
  * @param sig Signal number
  */
 void sigterm_handler(int sig);
 
 int main(int argc, char const *argv[]) {
     // Register the signal handler for SIGINT
     if (signal(SIGINT, sigint_handler) == SIG_ERR) {
         perror("Error: Failed to register SIGINT handler");
         exit(EXIT_FAILURE);
     }
     
     // Register the signal handler for SIGTERM (kill command or termination signal)
     if (signal(SIGTERM, sigterm_handler) == SIG_ERR) {
         perror("Error: Failed to register SIGTERM handler");
         exit(EXIT_FAILURE);
     }
     
     printf("Program started. Press Ctrl + C to send SIGINT, or send SIGTERM to terminate.\n");
 
     char input[256];  // Buffer to store user input
     fd_set readfds;   // File descriptor set for select()
     int retval;       // Return value from select()
 
     while (1) {
         // Clear the file descriptor set
         FD_ZERO(&readfds);
 
         // Add the standard input (keyboard) to the file descriptor set
         FD_SET(STDIN_FILENO, &readfds);
 
         // Call select() to wait for input or signal
         // When a signal is received, select() is interrupted and returns -1
         retval = select(STDIN_FILENO + 1, &readfds, NULL, NULL, NULL);
         
         // Check if select() encountered an error
         if (retval == -1) {
             // EINTR indicates the call was interrupted by a signal
             // This is not a fatal error, so we continue the loop
             if (errno == EINTR) {
                 continue;
             }
             
             perror("Error: select() failed");
             exit(EXIT_FAILURE);
         }
 
         // Check if there is input available on standard input (stdin)
         if (FD_ISSET(STDIN_FILENO, &readfds)) {
             // Read data from stdin when available
             if (fgets(input, sizeof(input), stdin) != NULL) {
                 // Remove trailing newline if present
                 size_t len = strlen(input);
                 if (len > 0 && input[len-1] == '\n') {
                     input[len-1] = '\0';
                 }
                 
                 printf("User input: %s\n", input);
             }
         }
     }
 
     return 0;
 }
 
 /**
  * Handler for SIGINT signal
  * Prints a message but doesn't exit the program
  */
 void sigint_handler(int sig) {
     printf("\nSIGINT received\n");
     
     // Re-register the signal handler (some systems reset it after each signal)
     signal(SIGINT, sigint_handler);
 }
 
 /**
  * Handler for SIGTERM signal
  * Prints a message and exits the program
  */
 void sigterm_handler(int sig) {
     printf("\nSIGTERM received. Exiting program...\n");
     exit(EXIT_SUCCESS);
 }