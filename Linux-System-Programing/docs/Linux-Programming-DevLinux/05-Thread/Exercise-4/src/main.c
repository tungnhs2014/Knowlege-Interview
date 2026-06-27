/**
 * @file main.c
 * @brief Program demonstrating parallel counting of even and odd numbers using threads
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <pthread.h>
 #include <time.h>
 
 #define ARRAY_SIZE 100     // Size of the array to be generated and analyzed
 
 // Global variables to store the array and counts
 int even_count = 0;        // Count of even numbers
 int odd_count = 0;         // Count of odd numbers
 int numbers[ARRAY_SIZE];   // Array to store random numbers
 
 /**
  * Thread function to count even numbers in the array
  * @param arg Thread arguments (not used)
  * @return NULL
  */
 void* count_even(void *arg);
 
 /**
  * Thread function to count odd numbers in the array
  * @param arg Thread arguments (not used)
  * @return NULL
  */
 void* count_odd(void *arg);
 
 int main(int argc, char const *argv[])
 {
     pthread_t even_thread, odd_thread;
     int ret;
 
     printf("Generating %d random numbers and counting even/odd numbers using threads\n", ARRAY_SIZE);
 
     // Initialize the random number generator
     srand(time(NULL));
 
     // Generate 100 random numbers between 1 and 100
     printf("Generated numbers: ");
     for (int i = 0; i < ARRAY_SIZE; i++) {
         numbers[i] = rand() % 100 + 1;
         
         // Print only the first 10 numbers to avoid cluttering the output
         if (i < 10) {
             printf("%d ", numbers[i]);
         } else if (i == 10) {
             printf("...");
         }
     }
     printf("\n");
 
     // Create the even counting thread
     ret = pthread_create(&even_thread, NULL, count_even, NULL);
     if (ret != 0) {
         fprintf(stderr, "Error: Failed to create even counting thread (error code: %d)\n", ret);
         exit(EXIT_FAILURE);
     }
 
     // Create the odd counting thread
     ret = pthread_create(&odd_thread, NULL, count_odd, NULL);
     if (ret != 0) {
         fprintf(stderr, "Error: Failed to create odd counting thread (error code: %d)\n", ret);
         exit(EXIT_FAILURE);
     }
 
     printf("Created two threads to count even and odd numbers\n");
 
     // Wait for both threads to finish
     if (pthread_join(even_thread, NULL) != 0) {
         fprintf(stderr, "Error: Failed to join even counting thread\n");
         exit(EXIT_FAILURE);
     }
     
     if (pthread_join(odd_thread, NULL) != 0) {
         fprintf(stderr, "Error: Failed to join odd counting thread\n");
         exit(EXIT_FAILURE);
     }
 
     printf("Both threads have completed\n");
 
     // Print the final counts
     printf("Total even numbers: %d\n", even_count);
     printf("Total odd numbers: %d\n", odd_count);
     printf("Total numbers: %d\n", even_count + odd_count);
 
     // Verify that the counts add up to the array size
     if (even_count + odd_count != ARRAY_SIZE) {
         fprintf(stderr, "Warning: Count mismatch - sum does not equal array size\n");
     }
 
     return 0;
 }
 
 /**
  * Implementation of thread function to count even numbers
  */
 void* count_even(void *arg) {
     for (int i = 0; i < ARRAY_SIZE; i++) {
         if (numbers[i] % 2 == 0) {
             even_count++;
         }
     }
     return NULL;
 }
 
 /**
  * Implementation of thread function to count odd numbers
  */
 void* count_odd(void *arg) {
     for (int i = 0; i < ARRAY_SIZE; i++) {
         if (numbers[i] % 2 != 0) {
             odd_count++;
         }
     }
     return NULL;
 }