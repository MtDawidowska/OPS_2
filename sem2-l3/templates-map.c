#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#define FILE_PATH "/tmp/mmap_example"
#define FILE_SIZE 4096
#define INT_ARRAY_SIZE 10
#define CHAR_ARRAY_SIZE 100

int main()
{
    // handling mapping a file
    int fd = open(FILE_PATH, O_RDWR | O_CREAT, 0666); // open
    if (fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, FILE_SIZE) == -1)
    { // change size
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    void *file_ptr = mmap(0, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); // map
    if (file_ptr == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    const char *message = "Hello, world!";
    memcpy(file_ptr, message, strlen(message) + 1); // input

    // handling mapping an array
    int *int_array = mmap(0, INT_ARRAY_SIZE * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (int_array == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < INT_ARRAY_SIZE; i++)
    {
        int_array[i] = i;
    }

    // another array
    char *char_array = mmap(0, CHAR_ARRAY_SIZE * sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (char_array == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    snprintf(char_array, CHAR_ARRAY_SIZE, "The first three integers are: %d, %d, %d", int_array[0], int_array[1], int_array[2]); // formatting string

    // Clean up
    if (munmap(file_ptr, FILE_SIZE) == -1)
    {
        perror("munmap");
        exit(EXIT_FAILURE);
    }

    if (munmap(int_array, INT_ARRAY_SIZE * sizeof(int)) == -1)
    {
        perror("munmap");
        exit(EXIT_FAILURE);
    }

    if (munmap(char_array, CHAR_ARRAY_SIZE * sizeof(char)) == -1)
    {
        perror("munmap");
        exit(EXIT_FAILURE);
    }

    if (close(fd) == -1) // can be done right after mapping
    {
        perror("close");
        exit(EXIT_FAILURE);
    }

    return 0;
}