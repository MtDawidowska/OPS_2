/******STAGE2******
Server reads the first message from PID_s queue.
Sends the first answer back to the client. Ignores all errors. */

#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_MSG_SIZE 100

volatile sig_atomic_t should_exit = 0;

void sigint_handler(int signum)
{
    should_exit = 1;
}

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

void usage(void)
{
    fprintf(stderr, "no input\n");
    exit(EXIT_FAILURE);
}

void mq_reader(int sig, siginfo_t *info, void *p)
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
        if (0 == msg_prio) // 0 the message from client
        {
            int id, a, b;
            char response[MAX_MSG_SIZE];
            char client_mq[20];
            if (sscanf(buffer, "%d %d %d", &id, &a, &b) != 3)
                ERR("retreiving message");

            sprintf(response, "%d", a + b);
            sprintf(client_mq, "/%d", id);
            {
                mqd_t mqd;                         
                mqd = mq_open(client_mq, O_WRONLY); 

                if (mqd == (mqd_t)-1)
                    ERR("mq_open() response from server");

                if (mq_send(mqd, response, MAX_MSG_SIZE, 1) < 0) //prio 1 means response
                    ERR("mq_send() response from server");
            }
        }
        else
            printf("server received sth weird\n");
    }
}

int main(int argc, char **argv)
{
    if (argc != 1)
        usage();

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        return 1;
    }

    for (int i = 1; i < NSIG; i++)
    {
        if (i != SIGINT)
        {
            signal(i, SIG_IGN);
        }
    }

    mqd_t sum_queue, division_queue, modulo_queue;
    char sum_queue_name[20], division_queue_name[20], modulo_queue_name[20];
    sprintf(sum_queue_name, "/%d_s", getpid());
    sprintf(division_queue_name, "/%d_d", getpid());
    sprintf(modulo_queue_name, "/%d_m", getpid());

    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;

    sum_queue = mq_open(sum_queue_name, O_CREAT | O_RDWR, 0600, &attr);
    if (sum_queue_name < 0)
    {
        perror("mq_open()");
        return 1;
    }
    division_queue = mq_open(division_queue_name, O_CREAT | O_RDWR, 0600, &attr);
    if (division_queue < 0)
    {
        perror("mq_open()");
        return 1;
    }
    modulo_queue = mq_open(modulo_queue_name, O_CREAT | O_RDWR, 0600, &attr);
    if (modulo_queue < 0)
    {
        perror("mq_open()");
        return 1;
    }

    printf("Server %d: Created queue %s, %s, %s\n", getpid(), sum_queue_name, division_queue_name, modulo_queue_name);

    while (!should_exit)
    {
        sleep(1);
    }
    if (mq_close(sum_queue) < 0)
    {
        perror("mq_close()");
        return 1;
    }
    if (mq_close(division_queue) < 0)
    {
        perror("mq_close()");
        return 1;
    }
    if (mq_close(modulo_queue) < 0)
    {
        perror("mq_close()");
        return 1;
    }
}