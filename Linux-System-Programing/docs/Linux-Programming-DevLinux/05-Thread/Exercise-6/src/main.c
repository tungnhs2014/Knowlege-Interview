/**
 * @file main.c
 * @brief Program demonstrating multithreaded array summing with mutex synchronization
 */

 #include <stdio.h>      
 #include <stdlib.h>     
 #include <pthread.h>    // Library for POSIX threads
 #include <time.h>      
 
 // Constants
 #define ARRAY_SIZE 1000000  // Array size: 1 million elements
 #define NUM_THREADS 4       // Number of threads to use
 #define MAX_RANDOM_VALUE 100 // Maximum random value for array elements
 
 // Global variables (shared between threads)
 int array[ARRAY_SIZE];     // Integer array to sum
 long long total_sum = 0;   // Variable to store the total sum
 pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex to protect total_sum
 
 /**
  * Structure to pass thread-specific data
  */
 typedef struct {
     int start_index;  // Starting index for this thread's portion
     int end_index;    // Ending index for this thread's portion
     long long partial_sum; // Each thread's partial sum (for verification)
 } ThreadData;
 
 /**
  * Thread function to calculate sum of array portion
  * @param arg Pointer to thread data containing start and end indices
  * @return NULL
  */
 void* sum_array_portion(void* arg);
 
 int main() {
     pthread_t threads[NUM_THREADS];  // Array to store thread IDs
     ThreadData thread_data[NUM_THREADS];  // Array to store data passed to each thread
     int ret;
     long long verification_sum = 0;  // For verifying the result
 
     // Initialize random number generator with seed based on current time
     srand(time(NULL));
     
     // Initialize the array with random numbers from 0 to (MAX_RANDOM_VALUE-1)
     printf("Initializing array with %d random numbers...\n", ARRAY_SIZE);
     for (int i = 0; i < ARRAY_SIZE; i++) {
         array[i] = rand() % MAX_RANDOM_VALUE;
     }
     
     // Calculate the size of each array portion to be assigned to each thread
     int chunk_size = ARRAY_SIZE / NUM_THREADS;
     
     // Create and launch the threads
     printf("Creating %d threads to calculate sum...\n", NUM_THREADS);
     for (int i = 0; i < NUM_THREADS; i++) {
         // Set up the information for this thread
         thread_data[i].start_index = i * chunk_size;
         
         // Ensure the last thread processes all remaining elements
         // (in case ARRAY_SIZE is not divisible by NUM_THREADS)
         if (i == NUM_THREADS - 1) {
             thread_data[i].end_index = ARRAY_SIZE;
         } else {
             thread_data[i].end_index = (i + 1) * chunk_size;
         }
         
         thread_data[i].partial_sum = 0;
         
         // Create a new thread and pass the corresponding data
         ret = pthread_create(&threads[i], NULL, sum_array_portion, &thread_data[i]);
         if (ret != 0) {
             fprintf(stderr, "Error: Failed to create thread %d (error code: %d)\n", i, ret);
             exit(EXIT_FAILURE);
         }
     }
     
     // Wait for all threads to complete
     for (int i = 0; i < NUM_THREADS; i++) {
         ret = pthread_join(threads[i], NULL);
         if (ret != 0) {
             fprintf(stderr, "Error: Failed to join thread %d (error code: %d)\n", i, ret);
             exit(EXIT_FAILURE);
         }
         // Sum the partial results for verification
         verification_sum += thread_data[i].partial_sum;
     }
     
     printf("Sum of the array (multithreaded): %lld\n", total_sum);
     
     // Verify the result by checking against the partial sums
     printf("Sum of partial results (verification): %lld\n", verification_sum);
     
     // Destroy the mutex when no longer needed
     pthread_mutex_destroy(&mutex);
     
     return 0;
 }
 
 /**
  * Thread function implementation
  * Calculates sum of array portion and updates global total
  */
 void* sum_array_portion(void* arg) {
     ThreadData* data = (ThreadData*)arg;
     long long local_sum = 0;
     
     // Calculate the sum of the assigned array portion
     for (int i = data->start_index; i < data->end_index; i++) {
         local_sum += array[i];
     }
     
     // Store the partial sum in the thread data for verification
     data->partial_sum = local_sum;
     
     // ----- CRITICAL SECTION BEGINS -----
     // Lock the mutex before accessing the shared variable total_sum
     pthread_mutex_lock(&mutex);
     
     // Add the local sum to the global sum (shared between threads)
     total_sum += local_sum;
     
     // Unlock the mutex after completing the update to the shared variable
     pthread_mutex_unlock(&mutex);
     // ----- CRITICAL SECTION ENDS -----
     
     return NULL;
 }