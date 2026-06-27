/**
 * @file main.c
 * @brief Program demonstrating thread synchronization with condition variables
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <pthread.h>
 #include <unistd.h>
 #include <time.h>
 
 #define NUM_ITERATIONS 10   // Number of producer-consumer iterations
 
 // Global shared variables
 int data = 0;              // Data produced by producer thread
 int data_ready = 0;        // Flag indicating if data is ready to be consumed
 
 // Synchronization primitives
 pthread_mutex_t mutex;     // Mutex for protecting shared data
 pthread_cond_t cond;       // Condition variable for signaling data availability
 
 /**
  * Producer thread function - generates random numbers
  * @param arg Thread arguments (not used)
  * @return NULL
  */
 void* producer(void *arg);
 
 /**
  * Consumer thread function - reads and prints data
  * @param arg Thread arguments (not used)
  * @return NULL
  */
 void* consumer(void *arg);
 
 int main(int argc, char const *argv[])
 {
     pthread_t producer_thread, consumer_thread;
     int ret;
 
     // Initialize the random number generator
     srand(time(NULL));
 
     // Initialize the mutex and condition variable
     if (pthread_mutex_init(&mutex, NULL) != 0) {
         fprintf(stderr, "Error: Failed to initialize mutex\n");
         exit(EXIT_FAILURE);
     }
     
     if (pthread_cond_init(&cond, NULL) != 0) {
         fprintf(stderr, "Error: Failed to initialize condition variable\n");
         pthread_mutex_destroy(&mutex);
         exit(EXIT_FAILURE);
     }
 
     printf("Starting producer-consumer demonstration with condition variables\n");
     printf("Will perform %d iterations of produce-consume cycle\n", NUM_ITERATIONS);
 
     // Create the producer thread
     ret = pthread_create(&producer_thread, NULL, producer, NULL);
     if (ret != 0) {
         fprintf(stderr, "Error: Failed to create producer thread (error code: %d)\n", ret);
         pthread_mutex_destroy(&mutex);
         pthread_cond_destroy(&cond);
         exit(EXIT_FAILURE);
     }
     
     // Create the consumer thread
     ret = pthread_create(&consumer_thread, NULL, consumer, NULL);
     if (ret != 0) {
         fprintf(stderr, "Error: Failed to create consumer thread (error code: %d)\n", ret);
         pthread_mutex_destroy(&mutex);
         pthread_cond_destroy(&cond);
         exit(EXIT_FAILURE);
     }
 
     // Wait for both threads to finish
     pthread_join(producer_thread, NULL);
     pthread_join(consumer_thread, NULL);
 
     printf("\nAll iterations completed successfully\n");
 
     // Destroy the mutex and condition variable
     pthread_mutex_destroy(&mutex);
     pthread_cond_destroy(&cond);
 
     return 0;
 }
 
 /**
  * Producer thread function implementation
  * Generates random numbers and notifies the consumer
  */
 void* producer(void *arg) {
     for (int i = 0; i < NUM_ITERATIONS; i++) {
         // Lock the mutex before accessing shared data
         pthread_mutex_lock(&mutex);
 
         // Generate a random number between 1 and 10
         data = rand() % 10 + 1; 
 
         printf("Producer: Generated data = %d\n", data);
         
         // Mark data as ready for consumption
         data_ready = 1;
         
         // Signal the consumer that data is available 
         pthread_cond_signal(&cond);
 
         // Unlock the mutex
         pthread_mutex_unlock(&mutex);
 
         // Sleep to simulate work
         sleep(1);
     }
     
     return NULL;
 }
 
 /**
  * Consumer thread function implementation
  * Waits for data from producer and processes it
  */
 void* consumer(void *arg) {
     int values[NUM_ITERATIONS]; // Array to store consumed values
 
     for (int i = 0; i < NUM_ITERATIONS; i++) {
         // Lock the mutex before accessing shared data
         pthread_mutex_lock(&mutex);
         
         // Wait until the producer signals that data is ready
         while (data_ready == 0) {
              pthread_cond_wait(&cond, &mutex);
         }
 
         // Store and display the consumed value
         values[i] = data;
         printf("Consumer: Consumed data = %d\n", data);
 
         // Reset the data ready flag
         data_ready = 0;
 
         // Unlock the mutex after consuming the data
         pthread_mutex_unlock(&mutex);
     }
     
     // Print all consumed values
     printf("\nAll values read:\n");
     for (int i = 0; i < NUM_ITERATIONS; i++) {
         printf("Value: %d\t", values[i]);
     }
     printf("\n");
 
     return NULL;
 }