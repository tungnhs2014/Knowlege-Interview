/**
 * @file log.h
 * @brief Interface for the logging system
 */

#ifndef _LOG_H_
#define _LOG_H_

#include <unistd.h>

/**
 * @brief Start the log process
 * 
 * This is function creates a child process using fork that handles
 * logging for the system. The child process reads log messages from 
 * a FIFO and write them to a log file
 * 
 * @param fifo_path Path to the FIFO for log messages
 * @param log_file Path to the log file
 * @return Process ID of the child process, or -1 on failure
 */
 pid_t log_start(const char *fifo_path, const char *log_file);

/**
 * @brief Open the log FIFO for writing
 * 
 * This is fucntion opens the log FIFO for writing and create it
 * if it doesn't exist already.
 * 
 * @param fifo_path Path to the FIFO for log messages
 * @return File descriptor for the FIFO, or -1 on failure
 */
int log_open_fifo(const char *fifo_path);

#endif
