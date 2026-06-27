/**
 * @file orphan.c
 * @brief Program demonstrating the creation of an orphan process
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <sys/wait.h> 
 
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
         // Code executed by child process
         printf("Child process started: PID = %d, PPID = %d\n", getpid(), getppid());
         printf("Child will sleep for 10 seconds while parent exits...\n");
         
         /**
          * The child process sleeps for 10 seconds
          * This delay allows parent process to exit before the child process
          * Making the child an orphan process that will be adopted by init (PID = 1) */
         sleep(10);
         
         // After sleep, the parent has exited and init has adopted this process
         printf("Orphan process running, now adopted by init (PID = 1)\n");
         printf("Current PID = %d, PPID = %d\n", getpid(), getppid());
         printf("Verify with: ps -ef | grep orphan\n");
         
         exit(0);
     }
     else {
         // Code executed by parent process
         printf("Parent process started: PID = %d\n", getpid());
         printf("Parent will exit immediately, abandoning child...\n");
         
         // Parent exits immediately, leaving the child as an orphan
         exit(0);
     }
 
     return 0;
 }