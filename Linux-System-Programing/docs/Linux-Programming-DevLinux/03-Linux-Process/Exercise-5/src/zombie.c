/**
 * @file zombie.c
 * @brief Program demonstrating the creation of a zombie process
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
         printf("Child process (Zombie) started: PID = %d\n", getpid());
         printf("Child process exiting now...\n");
         
         // Child exits immediately, but parent doesn't collect its status
         exit(0);
     }
     else {
         // Code executed by parent process
         printf("Parent process (Zombie creator) PID = %d\n", getpid());
         printf("Parent process not calling wait() - Child will become zombie\n");
         printf("Run 'ps aux | grep -iE zombie' in another terminal to observe\n");
 
         /*
          * The parent process enters an infinite loop.
            Because it never calls wait() to collect the child's exit status,
            the child process remains in the process table as a zombie. */ 
         while(1) {
             sleep(10); // Sleep to reduce CPU usage
         }
     }
 
     return 0;
 }