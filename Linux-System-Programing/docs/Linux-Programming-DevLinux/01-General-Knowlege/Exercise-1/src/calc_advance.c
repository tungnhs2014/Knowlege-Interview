/**
 * @file calc_advance.c
 * @brief Implementation of advanced calculation operations
 */

 #include "calc_advance.h"

 // Implementation of power function using iteration
 int power(int base, int exp) {
     int result = 1;
     
     for (int i = 0; i < exp; i++) {
         result *= base;
     }
     
     return result;
 }
 
 // Implementation of factorial function using recursion
 long long factorial(int n) {
     // Base cases: 0! = 1 and 1! = 1
     if (n == 0 || n == 1) {
         return 1;
     }
     
     // Recursive case: n! = n * (n-1)!
     return n * factorial(n - 1);
 }