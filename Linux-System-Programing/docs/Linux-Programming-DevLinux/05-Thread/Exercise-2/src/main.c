/**
 * @file main.c
 * @brief Program demonstrating thread synchronization with mutex
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <pthread.h>
 #include <unistd.h>
 
 // Constants
 #define NUM_THREADS     3       // Number of threads to be created
 #define NUM_ITERATIONS  1000000 // Number of increments per thread
 
 // Global variables
 volatile long long counter = 0;  // Global counter variable shared between threads
 pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex for thread synchronization
 
 /**
  * Thread function that increments the global counter
  * @param arg Thread arguments
  * @return NULL
  */
 void* thread_counter(void* arg);
 
 int main(int argc, char const *argv[])
 {
     pthread_t threads[NUM_THREADS];
     int ret;
 
     printf("Starting thread synchronization example...\n");
     printf("Creating %d threads, each incrementing counter %d times\n", 
            NUM_THREADS, NUM_ITERATIONS);
 
     // Create threads
     for (int i = 0; i < NUM_THREADS; i++) {
         ret = pthread_create(&threads[i], NULL, thread_counter, NULL);
         if (ret != 0) {
             fprintf(stderr, "Error: Failed to create thread %d (error code: %d)\n", i + 1, ret);
             exit(EXIT_FAILURE);
         }
     }
 
     // Wait for all threads to complete using pthread_join
     for (int i = 0; i < NUM_THREADS; i++) {
         ret = pthread_join(threads[i], NULL);
 
         // Check if joining the thread was successful
         if(ret != 0) {
             fprintf(stderr, "Error: Failed to join thread %d (error code: %d)\n", i + 1, ret);
             exit(EXIT_FAILURE);
         }
     }
     
     // Print the final value of the counter after all threads have completed
     printf("Final counter value: %lld\n", counter);
     printf("Expected value: %lld\n", (long long)NUM_THREADS * NUM_ITERATIONS);
 
     // Destroy the mutex to release resources
     pthread_mutex_destroy(&mutex);
 
     return 0;
 }
 
 /**
  * Thread function implementation
  * Each thread increments the counter NUM_ITERATIONS times
  * Uses mutex to ensure thread safety during the increment operation
  */
 void* thread_counter(void* arg) {
    for (int i = 0; i < NUM_ITERATIONS; i++) {
         // Lock the mutex before accessing the shared counter
         pthread_mutex_lock(&mutex);
         
         // Critical section - increment the counter
         counter++;
         
         // Unlock the mutex to allow other threads to access the counter
         pthread_mutex_unlock(&mutex);
    }
    
    // Exit the thread
    pthread_exit(NULL);
 }