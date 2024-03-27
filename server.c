/******STAGE 1******
The server creates its queues and displays the names of the queues.
 After 1 second, it destroys those queues and terminates. */
 
 #define _GNU_SOURCE
#include <errno.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

void usage(void)
{
    fprintf(stderr, "no input\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc != 1)
        usage();
    
    mqd_t sum_queue, division_queue, modulo_queue;
    char sum_queue_name[20], division_queue_name[20], modulo_queue_name[20];
    sprintf(sum_queue_name, "/%d_s", getpid());
    sprintf(division_queue_name, "/%d_d", getpid());
    sprintf(modulo_queue_name, "/%d_m", getpid());

    sum_queue = mq_open(sum_queue_name, O_CREAT | O_RDWR, 0600, NULL);
    if (sum_queue_name < 0) {
        perror("mq_open()");
        return 1;
    }
    division_queue = mq_open(division_queue_name, O_CREAT | O_RDWR, 0600, NULL);
    if (division_queue < 0) {
        perror("mq_open()");
        return 1;
    }
    modulo_queue = mq_open(modulo_queue_name, O_CREAT | O_RDWR, 0600, NULL);
    if (modulo_queue < 0) {
        perror("mq_open()");
        return 1;
    }

    sleep(1);

    printf("Server %d: Created queue %s, %s, %s\n", getpid(), sum_queue_name, division_queue_name, modulo_queue_name);

    if (mq_close(sum_queue) < 0) {
        perror("mq_close()");
        return 1;
    }
    if (mq_close(division_queue) < 0) {
        perror("mq_close()");
        return 1;
    }
    if (mq_close(modulo_queue) < 0) {
        perror("mq_close()");
        return 1;
    }
}