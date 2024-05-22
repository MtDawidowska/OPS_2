#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <errno.h>

typedef struct {
    pthread_mutex_t mutex;
    int data;
} SharedData;

int main() {
    SharedData *sharedData = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (sharedData == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
    pthread_mutex_init(&sharedData->mutex, &attr);

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {

        int ret = pthread_mutex_lock(&sharedData->mutex);
        if (ret == EOWNERDEAD) {
            pthread_mutex_consistent(&sharedData->mutex);
        } else if (ret != 0) {
            perror("pthread_mutex_lock");
            exit(EXIT_FAILURE);
        }

        sharedData->data++;

        pthread_mutex_unlock(&sharedData->mutex);

        exit(EXIT_SUCCESS);
    } else {

        int ret = pthread_mutex_lock(&sharedData->mutex);
        if (ret == EOWNERDEAD) {

            pthread_mutex_consistent(&sharedData->mutex);
        } else if (ret != 0) {
            perror("pthread_mutex_lock");
            exit(EXIT_FAILURE);
        }

        sharedData->data++;

        pthread_mutex_unlock(&sharedData->mutex);

        wait(NULL);
    }

    // Clean up
    pthread_mutex_destroy(&sharedData->mutex);
    munmap(sharedData, sizeof(SharedData));

    return 0;
}