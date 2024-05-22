#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#define SEM_NAME "/sem_example"

int main() {
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {

        if (sem_wait(sem) == -1) {
            perror("sem_wait");
            exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);
    } else {

        if (sem_post(sem) == -1) {
            perror("sem_post");
            exit(EXIT_FAILURE);
        }

        if (wait(NULL) == -1) { 
            perror("wait");
            exit(EXIT_FAILURE);
        }

        // Clean up
        if (sem_close(sem) == -1) {
            perror("sem_close");
            exit(EXIT_FAILURE);
        }

        if (sem_unlink(SEM_NAME) == -1) {
            perror("sem_unlink");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}