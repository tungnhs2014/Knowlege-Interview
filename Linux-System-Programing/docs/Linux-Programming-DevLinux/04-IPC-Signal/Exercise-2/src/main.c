/**
 * @file main.c
 * @brief Program implementing a timer using SIGALRM signals
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <signal.h>
 
 /**
  * Global variable to keep track of the timer count
  * Using volatile as it's modified in a signal handler */
 volatile int timer_count = 0;
 
 /**
  * Signal handler function for SIGALRM
  * @param sig Signal number
  */
 void alarm_handler(int sig);
 
 int main(int argc, char const *argv[])
 {
     // Register the signal handler for SIGALRM
     if (signal(SIGALRM, alarm_handler) == SIG_ERR) {
         perror("Error: Failed to register signal handler");
         exit(EXIT_FAILURE);
     }
 
     printf("Timer program started. Will count to 10 seconds.\n");
     
     // Set an alarm to trigger SIGALRM after 1 second
     alarm(1);
 
     // The actual timing is handled by the alarm and signal handler
     while (1) {
         pause(); // Waits for any signal (More efficient than sleep(1))
     }
     
     return 0;
 }
 
 /**
  * Handler for SIGALRM signal
  * Counts the elapsed time and exits after 10 seconds
  */
 void alarm_handler(int sig) {
     timer_count++;
     printf("\nTime: %d seconds\n", timer_count);
 
     // Check if the timer count has reached 10 seconds
     if (timer_count >= 10) {
         printf("Timer reached 10 seconds. Exiting program...\n");
         exit(EXIT_SUCCESS);
     }
     
     /**
      * Reset the alarm for the next second to keep counting
      * Without this, the alarm would trigger only once */
     alarm(1);
 }