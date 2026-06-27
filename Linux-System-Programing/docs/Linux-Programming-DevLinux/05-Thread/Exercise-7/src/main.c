/**
 * @file main.c
 * @brief Program demonstrating Producer-Consumer pattern with threads and synchronization
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <pthread.h>
 #include <time.h>
 
 // Constants
 #define NUM_PRODUCERS       10   // Number of producer threads
 #define NUM_CONSUMERS       5    // Number of consumer threads
 #define BUFFER_SIZE         5    // Size of the shared buffer
 #define PRODUCER_ITERATIONS 10   // Number of items each producer will produce
 #define CONSUMER_ITERATIONS 20   // Number of items each consumer will consume
 
 // Arrays to store thread IDs for producers and consumers
 pthread_t producer_threads[NUM_PRODUCERS];
 pthread_t consumer_threads[NUM_CONSUMERS];
 
 // Thread ID arrays
 int producer_ids[NUM_PRODUCERS];
 int consumer_ids[NUM_CONSUMERS];
 
 // Synchronization primitives
 pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;       // Protects shared buffer
 pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;     // Signaled when buffer is not empty
 pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;      // Signaled when buffer is not full
 
 // Shared buffer and related variables
 int buffer[BUFFER_SIZE];   // Circular buffer to store items
 int insert_pos = 0;        // Position where next item will be inserted
 int remove_pos = 0;        // Position where next item will be removed
 int item_count = 0;        // Number of items currently in buffer
 
 /**
  * Producer thread function - generates items and puts them in the buffer
  * @param arg Pointer to producer ID
  * @return NULL
  */
 void* producer(void *arg);
 
 /**
  * Consumer thread function - takes items from the buffer and processes them
  * @param arg Pointer to consumer ID
  * @return NULL
  */
 void* consumer(void *arg);
 
 int main(int argc, char const *argv[])
 {
     int ret;
     
     // Initialize random number generator
     srand(time(NULL));
     
     printf("Starting Producer-Consumer demonstration\n");
     printf("Creating %d producers and %d consumers with buffer size %d\n\n", 
            NUM_PRODUCERS, NUM_CONSUMERS, BUFFER_SIZE);
     
     // Initialize buffer with -1 (indicating empty slots)
     for (int i = 0; i < BUFFER_SIZE; i++) {
         buffer[i] = -1;
     }
 
     // Create producer threads
     for (int i = 0; i < NUM_PRODUCERS; i++) {
         producer_ids[i] = i;  // Store the ID
         ret = pthread_create(&producer_threads[i], NULL, producer, &producer_ids[i]);
         if (ret != 0) {
             fprintf(stderr, "Error: Failed to create producer thread %d (error code: %d)\n", i, ret);
             exit(EXIT_FAILURE);
         }
     }
 
     // Create consumer threads
     for (int i = 0; i < NUM_CONSUMERS; i++) {
         consumer_ids[i] = i;  // Store the ID
         ret = pthread_create(&consumer_threads[i], NULL, consumer, &consumer_ids[i]);
         if (ret != 0) {
             fprintf(stderr, "Error: Failed to create consumer thread %d (error code: %d)\n", i, ret);
             exit(EXIT_FAILURE);
         }
     }
 
     // Wait for all producer threads to finish
     for (int i = 0; i < NUM_PRODUCERS; i++) {
         ret = pthread_join(producer_threads[i], NULL);
         if (ret != 0) {
             fprintf(stderr, "Error: Failed to join producer thread %d (error code: %d)\n", i, ret);
             exit(EXIT_FAILURE);
         }
     }
     
     // Wait for all consumer threads to finish
     for (int i = 0; i < NUM_CONSUMERS; i++) {
         ret = pthread_join(consumer_threads[i], NULL);
         if (ret != 0) {
             fprintf(stderr, "Error: Failed to join consumer thread %d (error code: %d)\n", i, ret);
             exit(EXIT_FAILURE);
         }
     }
 
     printf("\nAll threads have completed\n");
     
     // Clean up synchronization primitives
     pthread_mutex_destroy(&mutex);
     pthread_cond_destroy(&not_empty);
     pthread_cond_destroy(&not_full);
 
     return 0;
 }
 
 /**
  * Implementation of producer thread function
  */
 void* producer(void *arg) {
     int id = *((int*)arg);
     
     for (int i = 0; i < PRODUCER_ITERATIONS; i++) {
         // Generate a random value between 0 and 99
         int item = rand() % 100;
         
         // Lock mutex to protect shared resources
         pthread_mutex_lock(&mutex);
 
         // Wait while the buffer is full
         while (item_count == BUFFER_SIZE) {
             pthread_cond_wait(&not_full, &mutex);
         }
         
         // Add value to the buffer
         buffer[insert_pos] = item;
         printf("Producer %d: Added item %d at position %d\n", id, item, insert_pos);
         
         // Update buffer state
         insert_pos = (insert_pos + 1) % BUFFER_SIZE;  // Circular buffer
         item_count++;
 
         // Signal that the buffer is not empty
         pthread_cond_signal(&not_empty);
         
         // Unlock the mutex
         pthread_mutex_unlock(&mutex);
         
         // Sleep briefly to simulate variable production time
         usleep(rand() % 50000);  // 0-50ms
     }
 
     printf("Producer %d: Finished producing items\n", id);
     return NULL;
 }
 
 /**
  * Implementation of consumer thread function
  */
 void* consumer(void *arg) {
     int id = *((int*)arg);
     
     for (int i = 0; i < CONSUMER_ITERATIONS; i++) {
         // Lock the mutex to protect shared resources
         pthread_mutex_lock(&mutex);
 
         // Wait while the buffer is empty
         while (item_count == 0) {
             pthread_cond_wait(&not_empty, &mutex);
         }
 
         // Retrieve the item from the buffer
         int item = buffer[remove_pos];
         buffer[remove_pos] = -1;  // Mark as empty
         
         printf("  Consumer %d: Removed item %d from position %d\n", id, item, remove_pos);
         
         // Update buffer state
         remove_pos = (remove_pos + 1) % BUFFER_SIZE;  // Circular buffer
         item_count--;
         
         // Signal that the buffer is not full
         pthread_cond_signal(&not_full);
         
         // Unlock the mutex
         pthread_mutex_unlock(&mutex);
         
         // Sleep briefly to simulate variable consumption time
         usleep(rand() % 100000);  // 0-100ms
     }
 
     printf("Consumer %d: Finished consuming items\n", id);
     return NULL;
 }