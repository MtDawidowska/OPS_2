/******Stage2******
The client reads 2 integers from stdin and sends a single message to the server.
 waits for the result and displays it.*/

#define _GNU_SOURCE
#include <errno.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>z
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

void mq_handler(int sig, siginfo_t *info, void *p)
{
    mqd_t *mq;
    char buffer[MAX_MSG_SIZE];
    unsigned msg_prio;

    mq = (mqd_t *)info->si_value.sival_ptr;

    static struct sigevent not ;
    not .sigev_notify = SIGEV_SIGNAL;
    not .sigev_signo = SIGRTMIN;
    not .sigev_value.sival_ptr = mq;
    if (mq_notify(*mq, &not ) < 0)
        ERR("mq_notify");

    for (;;)
    {
        if (mq_receive(*mq, (char *)buffer, MAX_MSG_SIZE, &msg_prio) < 1)
        {
            if (errno == EAGAIN)
                break;
            else
                ERR("mq_receive");
        }
        if (1 == msg_prio) // if the message is a response
            printf("[%d] received: %s\n",getpid(), buffer);
        else
            printf("client received sth weird\n");
    }
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

    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize =  MAX_MSG_SIZE;
    client_queue = mq_open(client_queue_name, O_CREAT | O_RDWR, 0600, &attr);
    if (client_queue < 0) {
        perror("mq_open()");
        return 1;
    }

    sleep(1);

    //enter two integers for sum
    printf("enter two integers for sum:\n");
    int a, b;
    scanf("%d %d", &a, &b);

    //send a and b to the server
    char message[MAX_MSG_SIZE];
    sprintf(message, "%d %d %d",getpid(), a, b);
    if (mq_send(client_queue, message, strlen(message), 0) < 0) { //0 means "to calculate"
        perror("mq_send()");
        return 1;
    }

    //receive the result from the server
    static struct sigevent noti;
    noti.sigev_notify = SIGEV_SIGNAL;
    noti.sigev_signo = SIGRTMIN;
    noti.sigev_value.sival_ptr = &client_queue;
    if (mq_notify(client_queue, &noti) < 0)
        ERR("mq_notify");

    if (mq_close(client_queue) < 0) {
        perror("mq_close()");
        return 1;
    }
}