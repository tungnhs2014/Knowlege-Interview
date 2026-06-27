/**
 * @file calc_basic.h
 * @brief Library for basic calculation operations
 */

 #ifndef _CALC_BASIC_H
 #define _CALC_BASIC_H
 
 /**
  * @brief Add two integers
  * @param a First integer
  * @param b Second integer
  * @return Sum of two integers
  */
 int add(int a, int b);
 
 /**
  * @brief Subtract second integer from first integer
  * @param a First integer
  * @param b Second integer
  * @return Difference between two integers
  */
 int subtract(int a, int b);
 
 /**
  * @brief Multiply two integers
  * @param a First integer
  * @param b Second integer
  * @return Product of two integers
  */
 int multiply(int a, int b);
 
 /**
  * @brief Divide first integer by second integer
  * @param a Numerator (integer)
  * @param b Denominator (integer)
  * @return Result of division (double)
  */
 double divide(int a, int b);
 
 #endif