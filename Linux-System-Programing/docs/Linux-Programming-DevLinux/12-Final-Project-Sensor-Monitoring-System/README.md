# Sensor Gateway System

## Overview
The Sensor Gateway System is a multi-threaded, multi-process application that collects temperature data from multiple sensor nodes via TCP connections, processes this data to determine temperature conditions, and stores it in an SQLite database. The system also maintains comprehensive logs of all activities.

## System Architecture
The system consists of two main processes:
1. **Main Process**: Contains three threads that handle different aspects of the system:
   - **Connection Manager Thread**: Manages TCP connections with sensor nodes
   - **Data Manager Thread**: Processes sensor data to calculate running averages and determine temperature conditions
   - **Storage Manager Thread**: Stores sensor data in an SQLite database

2. **Log Process**: A separate process that receives log events from the main process and writes them to a log file

Data is shared between the threads in the main process using a thread-safe shared buffer.

## Components
The system is divided into the following components:

- **config.h**: Configuration parameters for the entire system
- **sbuffer**: Thread-safe shared buffer implementation
- **connmgr**: Connection manager for handling TCP connections
- **datamgr**: Data manager for processing sensor data
- **sensor_db**: Interface with the SQLite database
- **log**: Logging functionality
- **main**: Main program logic

## Requirements
- GCC compiler
- SQLite3 development libraries
- POSIX-compliant operating system (Linux recommended)
