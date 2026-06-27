/**
 * @file main.c
 * @brief A program to read from or write to a file based on command-line arguments
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <fcntl.h>
 #include <unistd.h>
 #include <string.h>
 #include <sys/stat.h>
 
 #define FILE_PATH "file/"   // Directory where files are stored
 #define MAX_PATH_LENGTH 256 // Maximum length for file path
 
 int main(int argc, char *argv[]) 
 {
     // Check if the correct number of arguments is provided
     if (argc < 4) {
         fprintf(stderr, "Error: Too few arguments.\n");
         fprintf(stderr, "Usage: %s filename num-bytes [r/w] [content]\n", argv[0]);
         return -1;
     }
 
     // Extract command-line arguments
     char *filename = argv[1];
     int num_bytes = atoi(argv[2]);  // Convert number of bytes from string to integer
     char mode = argv[3][0];         // Extract mode ('r' for read, 'w' for write)
     
     // Content is required for write mode
     char *content = NULL;
     if (mode == 'w') {
         if (argc < 5) {
             fprintf(stderr, "Error: Content argument missing for write mode.\n");
             return -1;
         }
         content = argv[4];
     }
 
     // Construct full file path
     char filepath[MAX_PATH_LENGTH];
     snprintf(filepath, sizeof(filepath), "%s%s", FILE_PATH, filename);
     
     // Process based on mode
     if (mode == 'r') {
         // Open file for reading
         int fd = open(filepath, O_RDONLY);
         if (fd == -1) {
             fprintf(stderr, "Error: Cannot open file '%s' for reading\n", filepath);
             return -1;
         }
         
         // Allocate buffer for reading
         char *buffer = malloc(num_bytes + 1);
         if (buffer == NULL) {
             fprintf(stderr, "Error: Memory allocation failed\n");
             close(fd);
             return -1;
         }
         
         // Read from file
         int bytes_read = read(fd, buffer, num_bytes);
         if (bytes_read == -1) {
             fprintf(stderr, "Error: Failed to read from file '%s'\n", filepath);
             free(buffer);
             close(fd);
             return -1;
         }
 
         // Null-terminate and display the read content
         buffer[bytes_read] = '\0';
         printf("Read from file: %s\n", buffer);
 
         // Clean up
         free(buffer);
         close(fd);
     } 
     else if (mode == 'w') {
         // Open file for writing with appropriate flags
         int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
         if (fd == -1) {
             fprintf(stderr, "Error: Cannot open file '%s' for writing\n", filepath);
             return -1;
         }
         
         // Write to file
         int bytes_written = write(fd, content, num_bytes);
         if (bytes_written == -1) {
             fprintf(stderr, "Error: Failed to write to file '%s'\n", filepath);
             close(fd);
             return -1;
         }
 
         // Display success message
         printf("Written %d bytes to file: %s\n", bytes_written, content);
 
         // Clean up
         close(fd);
     }
     else {
         // Invalid mode provided
         fprintf(stderr, "Error: Invalid mode '%c'. Use 'r' for read or 'w' for write.\n", mode);
         return -1;
     }
     
     return 0;
 }