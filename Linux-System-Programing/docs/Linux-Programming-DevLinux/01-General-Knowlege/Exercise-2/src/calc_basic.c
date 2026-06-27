/**
 * @file calc_basic.c
 * @brief Implementation of basic calculation operations
 */

 #include <stdio.h>
 #include "calc_basic.h"
 
 // Implementation of basic calculator functions
 
 int add(int a, int b) {
     return a + b;
 }
 
 int subtract(int a, int b) {
     return a - b;
 }
 
 int multiply(int a, int b) {
     return a * b;
 }
 
 double divide(int a, int b) {
     // Check for division by zero
     if (b == 0) {
         printf("Error!! Division by zero is invalid\n");
         return -1;
     }
     return (double)a / b;
 }