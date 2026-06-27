/**
 * @file main.c
 * @brief Program demonstrating the behavior of O_APPEND flag with lseek
 */

 #include <stdio.h>
 #include <string.h>     
 #include <fcntl.h>      // For file control options (open, O_APPEND, O_CREAT)
 #include <unistd.h>     // For system calls (write, lseek, close)
 #include <sys/stat.h>   // For mkdir and file permissions
 
 int main(int argc, char const *argv[])
 {
     // Buffer containing the data to write
     char data_buffer[20] = "Linux Programming ";
     int file_descriptor;
     int seek_result;
     int bytes_written;
 
     // Create a directory to store our test file
     if (mkdir("file", 0777) != 0) {
         printf("ERROR!! mkdir failed\n");
         return -1;
     }
 
     // Open file with O_APPEND flag
     file_descriptor = open("file/linux.txt", O_RDWR | O_CREAT | O_APPEND, 0666);
     if (file_descriptor == -1) {
         printf("ERROR!! open() failed\n");
         return -1;
     }
     
     /**
      * Try to move the file pointer to the beginning
      * This will succeed, but won't affect where data is written */
     seek_result = lseek(file_descriptor, 0, SEEK_SET);
     if (seek_result == -1) {
         printf("ERROR!! lseek() failed\n");
         close(file_descriptor);
         return -1;
     }
 
     /**
      * Write to the file - data will be appended at the end
      * despite the previous lseek() call because of O_APPEND flag */
     bytes_written = write(file_descriptor, data_buffer, strlen(data_buffer));
     printf("Wrote %d bytes to linux.txt\n", bytes_written);
     
     // Clean up by closing the file
     close(file_descriptor);
 
     return 0;
 }