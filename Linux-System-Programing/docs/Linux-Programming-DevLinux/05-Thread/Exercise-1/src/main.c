/**
 * @file main.c
 * @brief Program demonstrating basic thread creation and execution with pthreads
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <pthread.h>
 #include <unistd.h>
 
 /**
  * Thread function that prints a message with its ID
  * @param arg Pointer to the thread's ID
  * @return NULL
  */
 void* thread_function(void* arg);
 
 int main(int argc, char const *argv[])
 {
     // Declare an array of thread IDs
     pthread_t threads[2];
     int thread_ids[2];
     int ret;
 
     // Create two threads
     for (int i = 0; i < 2; i++) {
         // Define a unique ID for each thread
         thread_ids[i] = i + 1;
 
         // Create a new thread and pass the ID as an argument
         ret = pthread_create(&threads[i], NULL, thread_function, (void*)&thread_ids[i]);
         
         // Check if thread creation was successful
         if (ret != 0) {
             fprintf(stderr, "Error: Failed to create thread %d (error code: %d)\n", i + 1, ret);
             exit(EXIT_FAILURE);
         }
 
         printf("Main: Created thread %d\n", i + 1);   
     }
 
     // Wait for all threads to complete
     for (int i = 0; i < 2; i++) {
         ret = pthread_join(threads[i], NULL);
 
         // Check if joining the thread was successful
         if(ret != 0) {
             fprintf(stderr, "Error: Failed to join thread %d (error code: %d)\n", i + 1, ret);
             exit(EXIT_FAILURE);
         }
     }
 
     printf("Main: All threads have completed execution\n");
     return 0;
 }
 
 /**
  * Thread function implementation
  * This function is executed by each thread and prints a message with its ID
  */
 void* thread_function(void* arg) {
     // Get the thread ID from the argument
     int thread_id = *((int*)arg);
 
     printf("Thread %d: Hello from thread!\n", thread_id);
     
     // Exit the thread
     pthread_exit(NULL);
 }