#define _GNU_SOURCE

#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <string.h>

#define SEM_NAME "/sem_example"
#define SEM_DONE "/sem_done"
#define SHM_NAME "/shm_example"
// version 1 is with children each opening the named semaphore on their own
// version 2 is with semaphore being shared in the shared memory

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s server_pid\n", name);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        usage("1 <= N <= 10; 5 <= M <= 10; N players, M rounds\n");
    }
    int N = atoi(argv[1]);
    int M = atoi(argv[2]);
    if (N < 1 || N > 10 || M < 2 || M > 10)
    {
        usage("N-num of children M-num of cards\n");
    }

    int shm_fd;
    int shm_size = N * 4;

    if ((shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR | O_TRUNC, 0666)) == -1)
        ERR("shm_open");
    if (ftruncate(shm_fd, shm_size) == -1)
        ERR("ftruncate");
    char *shm_ptr;
    if ((shm_ptr = (char *)mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
        ERR("mmap");

    int *shm_int_ptr = (int *)shm_ptr;
    // creating semaphores
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0600, 0);      // for server to announce new round
    sem_t *sem_done = sem_open(SEM_DONE, O_CREAT, 0600, 0); // for children to signal they have input the cards into shared memory

    // creating children
    for (int i = 0; i < N; i++)
    {
        switch (fork())
        {
        case 0:
        {
            srand(getpid());

            sem_t *sem_child = sem_open(SEM_NAME, 0);
            sem_t *sem_done = sem_open(SEM_DONE, O_CREAT, 0600, 0);

            int cards[M];
            int index;

            // Initialize all cards
            for (int j = 0; j < M; j++)
            {
                cards[i] = j + 1;
            }

            for (int k = 0; k < M; k++)
            {
                sem_wait(sem_child);

                index = rand() % M;
                while (cards[index] == -1)
                {
                    index = (index + 1) % M;
                }
                // printf("Child %d drew card of index %d\n", i, index + 1);
                shm_int_ptr[i] = index + 1; // Write to shared memory
                cards[index] = -1;
                sem_post(sem_done);
                sleep(1);
            }
            sem_close(sem_child);
            sem_close(sem_done);
            return EXIT_SUCCESS;
        }
        default:
            break;
        }
    }

    int points[N];
    int winners[N];
    for (int j = 0; j < N; j++)
    {
        points[j] = 0;
        winners[j] = 0;
    }
    int max;
    int num_winners;

    for (int round = 0; round < M; round++)
    {
        printf("Round %d:\n", round + 1);
        max = 0;
        num_winners = 1;

        for (int k = 0; k < N; k++) // round begins
        {
            if (sem_post(sem) == -1)
                ERR("sem_post");
        }

        for (int l = 0; l < N; l++) // waiting for players
        {
            if (sem_wait(sem_done) == -1)
            {
                ERR("sem_wait");
            }
            printf("Player %d wrote %d\n", l, shm_int_ptr[l]);
            winners[l] = 0;
        }
        if (msync(shm_ptr, N, MS_SYNC)) // "fflush"
            ERR("msync");

        for (int n = 0; n < N; n++) // counting points
        {
            if (shm_int_ptr[n] > max)
            {
                max = shm_int_ptr[n];
                num_winners = 1;
                winners[0] = n;
            }
            else if (shm_int_ptr[n] == max)
            {
                winners[num_winners] = n;
                num_winners++;
            }
        }

        int points_per_winner = N / num_winners;

        for (int o = 0; o < num_winners; o++)
        {
            points[winners[o]] += points_per_winner;
        }
    }

    for (int p = 0; p < N; p++)
    {
        printf("Player %d has %d points\n", p, points[p]);
    }

    // Clean up
    for (int w = 0; w < N; w++)
    {
        if (wait(NULL) == -1)
            ERR("wait");
    }
    sem_close(sem);
    sem_unlink(SEM_NAME);

    sem_close(sem_done);
    sem_unlink(SEM_DONE);

    if (munmap(shm_ptr, shm_size) == -1)
        ERR("munmap");

    if (close(shm_fd) == -1)
        ERR("close");

    if (shm_unlink(SHM_NAME) == -1)
        ERR("shm_unlink");

    return EXIT_SUCCESS;
}