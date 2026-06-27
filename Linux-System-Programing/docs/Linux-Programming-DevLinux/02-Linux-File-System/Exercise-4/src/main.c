/**
 * @file main.c
 * @brief Program demonstrating file creation and metadata retrieval using stat()
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <fcntl.h>
 #include <unistd.h>
 #include <time.h>
 #include <stdint.h>
 #include <string.h>
 
 int main() {
     // Define the file name to be created
     char *file_name = "file/test.txt";
 
     /**
      * Open the file with write permission
      * O_WRONLY: Open for writing only
      * O_CREAT: Create file if it does not exist
      * O_TRUNC: Truncate file to zero length if it already exists
      * 0644: File permissions (owner: read/write, group/others: read-only) */
     int fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644); 
     if (fd == -1) {
         fprintf(stderr, "Error creating file\n");
         return -1;
     }
     
     // Define the content to write into the file
     char buffer[] = "Hello, I'm Son Tung\n";
     
     // Write data to the file
     int bytes_written = write(fd, buffer, strlen(buffer));
     if (bytes_written == -1) {
         fprintf(stderr, "Error writing to file\n");
         close(fd);
         return -1;
     }
 
     // Close file after writing
     close(fd);
 
     // Declare a struct stat variable to store file information
     struct stat file_stat;
 
     // Get file metadata using stat() system call
     if (stat(file_name, &file_stat) < 0) {
         fprintf(stderr, "Error getting file information\n");
         return -1;
     }
     
     // Print file information
     printf("File Name: %s\n", file_name);
     printf("File Size: %ld bytes\n", file_stat.st_size);
     printf("I-node number: %ju\n", (uintmax_t)file_stat.st_ino);
     printf("Last status change:     %s", ctime(&file_stat.st_ctime));
     printf("Last file access:       %s", ctime(&file_stat.st_atime));
     printf("Last file modification: %s", ctime(&file_stat.st_mtime));
     
     // Determine and print the type of file
     printf("File Type: ");
     switch (file_stat.st_mode & S_IFMT) {
         case S_IFBLK:   printf("Block device\n");       break;
         case S_IFCHR:   printf("Character device\n");   break;
         case S_IFDIR:   printf("Directory\n");          break;
         case S_IFIFO:   printf("FIFO/pipe\n");          break;
         case S_IFLNK:   printf("Symbolic link\n");      break;
         case S_IFREG:   printf("Regular file\n");       break;
         case S_IFSOCK:  printf("Socket\n");             break;
         default:        printf("Unknown\n");
     }
 
     return 0;
 }