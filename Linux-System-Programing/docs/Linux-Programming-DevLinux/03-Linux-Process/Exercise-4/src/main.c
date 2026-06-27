/**
 * @file main.c
 * @brief Program demonstrating process exit status recording
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <time.h>
 #include <unistd.h>
 #include <sys/wait.h> 
 
 int main(int argc, char const *argv[])
 {
     pid_t child_pid; // Define a variable to store process ID
     int status;
 
     // Seed the random number generator
     srand(time(NULL));
 
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
 
         // Generate a random exit code between 0 and 99
         int exit_code = rand() % 100;
         printf("Child process exiting with code: %d\n", exit_code);
 
         // Uncomment to simulate child process not exiting normally
         // while(1); // This will require using kill -9 from another terminal
 
         // Exit child process with the generated exit code
         exit(exit_code);
     }
     else {
         // Parent process code
         printf("Parent process waiting for child (PID: %d) to finish...\n", child_pid);
         
         // Wait for the child process to finish and retrieve the status
         pid_t terminated_pid = wait(&status);
 
         if (terminated_pid == -1) {
             fprintf(stderr, "Error: Wait failed\n");
             exit(1);
         }
 
         printf("Parent process: child process PID %d terminated\n", terminated_pid);
 
         // Check if the child process exited normally
         if (WIFEXITED(status)) {
             printf("Child process exited normally with status: %d\n", WEXITSTATUS(status));
         } 
         else if (WIFSIGNALED(status)) {
             printf("Child process terminated by signal %d\n", WTERMSIG(status));
         }
         else {
             printf("Child process did not exit normally\n");
         }
     }
 
     return 0;
 }