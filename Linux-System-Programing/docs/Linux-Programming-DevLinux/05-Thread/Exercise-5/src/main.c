/**
 * @file main.c
 * @brief Program demonstrating thread synchronization with read-write locks
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <pthread.h>
 #include <unistd.h>
 
 #define NUM_READERS    5     // Number of reader threads
 #define NUM_WRITERS    2     // Number of writer threads
 #define NUM_ITERATIONS 5     // Number of operations each thread will perform
 
 // Global shared data
 int data = 0;                 // Shared data that will be read/written
 pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;  // Read-write lock
 
 /**
  * Reader thread function - reads shared data without modifying it
  * @param arg Pointer to the thread ID
  * @return NULL
  */
 void* read_thread(void *arg);
 
 /**
  * Writer thread function - modifies shared data
  * @param arg Pointer to the thread ID
  * @return NULL
  */
 void* write_thread(void *arg);
 
 int main() {
     pthread_t readers[NUM_READERS], writers[NUM_WRITERS];
     int reader_ids[NUM_READERS], writer_ids[NUM_WRITERS];
     int ret;
 
     printf("Starting read-write lock demonstration\n");
     printf("Creating %d reader threads and %d writer threads\n", NUM_READERS, NUM_WRITERS);
     printf("Each thread will perform %d iterations\n\n", NUM_ITERATIONS);
 
     // Create writer threads
     for (int i = 0; i < NUM_WRITERS; i++) {
         writer_ids[i] = i + 1;
         ret = pthread_create(&writers[i], NULL, write_thread, &writer_ids[i]);
         if (ret != 0) {
             fprintf(stderr, "Error: Failed to create writer thread %d (error code: %d)\n", i + 1, ret);
             exit(EXIT_FAILURE);
         }
     }
 
     // Create reader threads
     for (int i = 0; i < NUM_READERS; i++) {
         reader_ids[i] = i + 1;
         ret = pthread_create(&readers[i], NULL, read_thread, &reader_ids[i]);
         if (ret != 0) {
             fprintf(stderr, "Error: Failed to create reader thread %d (error code: %d)\n", i + 1, ret);
             exit(EXIT_FAILURE);
         }
     }
 
     // Wait for all writer threads to complete
     for (int i = 0; i < NUM_WRITERS; i++) {
         ret = pthread_join(writers[i], NULL);
         if (ret != 0) {
             fprintf(stderr, "Error: Failed to join writer thread %d (error code: %d)\n", i + 1, ret);
             exit(EXIT_FAILURE);
         }
     }
     
     // Wait for all reader threads to complete
     for (int i = 0; i < NUM_READERS; i++) {
         ret = pthread_join(readers[i], NULL);
         if (ret != 0) {
             fprintf(stderr, "Error: Failed to join reader thread %d (error code: %d)\n", i + 1, ret);
             exit(EXIT_FAILURE);
         }
     }
 
     // Clean up and show the result
     pthread_rwlock_destroy(&rwlock);
     printf("\nAll threads completed execution\n");
     printf("Final value of data: %d\n", data);
     printf("Expected value: %d\n", NUM_WRITERS * NUM_ITERATIONS);
     
     return 0;
 }
 
 /**
  * Reader thread implementation
  * Acquires a read lock, reads the shared data, and releases the lock
  */
 void* read_thread(void *arg) {
     int thread_id = *(int*)arg;
 
     for (int i = 0; i < NUM_ITERATIONS; i++) {
         // Acquire read lock - multiple readers can hold this lock simultaneously
         pthread_rwlock_rdlock(&rwlock);
 
         // Critical section - read the shared data
         printf("Reader %d reads data: %d\n", thread_id, data);
 
         // Release the lock
         pthread_rwlock_unlock(&rwlock);
 
         // Sleep to simulate processing time and allow other threads to run
         usleep(100000);  // 100 milliseconds
     }
     
     return NULL;
 }
 
 /**
  * Writer thread implementation
  * Acquires a write lock, modifies the shared data, and releases the lock
  */
 void* write_thread(void *arg) {
     int thread_id = *(int*)arg;
 
     for (int i = 0; i < NUM_ITERATIONS; i++) {
         // Acquire write lock - only one thread can hold this lock
         pthread_rwlock_wrlock(&rwlock);
 
         // Critical section - modify the shared data
         data++;  
         printf("Writer %d update data to: %d\n", thread_id, data);
         
         // Release the lock
         pthread_rwlock_unlock(&rwlock);
 
         // Sleep to simulate processing time and allow other threads to run
         usleep(200000);  // 200 milliseconds
     }
     
     return NULL;
 }