/******Stage1******
The client process creates its own queue, waits for 1 second, 
destroys its queue, and terminates.*/

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

#define MAX_MSG_SIZE 100
#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

void usage(void)
{
    fprintf(stderr, "one input arg - server's PID\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    int n;
    if (argc != 2)
        usage();
    n = atoi(argv[1]);
    

    mqd_t client_queue;
    char client_queue_name[20], sum_queue_name[20], division_queue_name[20], modulo_queue_name[20];
    sprintf(client_queue_name, "/%d", getpid());
    sprintf(sum_queue_name, "/%d_s", n);
    sprintf(division_queue_name, "/%d_d", n);
    sprintf(modulo_queue_name, "/%d_m", n);

    client_queue = mq_open(client_queue_name, O_CREAT | O_RDWR, 0600, NULL);
    if (client_queue < 0) {
        perror("mq_open()");
        return 1;
    }

    sleep(1);

    printf("Client %d: Created queue %s\n", getpid(), client_queue_name);

    if (mq_close(client_queue) < 0) {
        perror("mq_close()");
        return 1;
    }
}