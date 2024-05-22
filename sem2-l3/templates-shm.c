#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define SHM_NAME "/shm_example"
#define SHM_SIZE 4096
#define NAME_SIZE 32

int main()
{
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, SHM_SIZE) == -1)
    {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    void *ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    memcpy(ptr, SHM_NAME, NAME_SIZE);

    switch (fork())
    {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
        case 0:
            char name[NAME_SIZE];
            memcpy(name, ptr, NAME_SIZE);

            int shm_fd_child = shm_open(name, O_RDONLY, 0666); // open
            if (shm_fd_child == -1)
            {
                perror("shm_open");
                exit(EXIT_FAILURE);
            }

            void *ptr_child = mmap(0, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd_child, 0); // map
            if (ptr_child == MAP_FAILED)
            {
                perror("mmap");
                exit(EXIT_FAILURE);
            }

            // Clean up
            if (munmap(ptr_child, SHM_SIZE) == -1)
            {
                perror("munmap");
                exit(EXIT_FAILURE);
            }

            if (close(shm_fd_child) == -1)
            {
                perror("close");
                exit(EXIT_FAILURE);
            }

            exit(EXIT_SUCCESS);
            ;

        default:
            break;
    }
    // in parent process

    // Wait for the child process to finish
    if (wait(NULL) == -1)
    {
        perror("wait");
        exit(EXIT_FAILURE);
    }

    // Clean up
    if (munmap(ptr, SHM_SIZE) == -1)
    {
        perror("munmap");
        exit(EXIT_FAILURE);
    }

    if (close(shm_fd) == -1)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }

    if (shm_unlink(SHM_NAME) == -1)
    {
        perror("shm_unlink");
        exit(EXIT_FAILURE);
    }

    return 0;
}