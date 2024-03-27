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

#define LIFE_SPAN 10
#define MAX_NUM 10

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t should_return = 0;
volatile sig_atomic_t num_children = 0;

void sethandler(void (*f)(int, siginfo_t *, void *), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void thread_routine(union sigval sv) {
    //printf("Notification thread starts!\n");

    mqd_t *pin = (mqd_t *) sv.sival_ptr;
    uint8_t ni;
    unsigned int prio;

    // Restore notification
    struct sigevent not;
    memset(&not, 0, sizeof(not));
    not.sigev_notify = SIGEV_THREAD;
    not.sigev_notify_function = thread_routine;
    not.sigev_notify_attributes = NULL; // Thread creation attributes
    not.sigev_value.sival_ptr = pin; // Thread routine argument
   
      if (mq_notify(*pin, &not) < 0) {
      perror("mq_notify()");
      exit(1);
    }

    for (;;)
    {
        //printf("before receiving in thread\n");
        if (mq_receive(*pin, (char *)&ni, 1, &prio) < 1)
        {
        //printf("errno %d, prio %d\n",errno,prio);
            if (errno == EAGAIN )
            {
                if( num_children == 0) return;
                    continue;
            }
            else
                ERR("mq_receive");
        }
        if (0 == prio)
            printf("MQ: got timeout from %d.\n", ni);
        else
            printf("MQ:%d is a bingo number!\n", ni);
            
    }
}

void sigchld_handler(int sig, siginfo_t *s, void *p)
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        num_children--;

        if (num_children == 0) {
            should_return = 1;
            break;
        }
    }
}

void child_work(int n, mqd_t pin, mqd_t pout)
{
    int life;
    uint8_t ni;
    uint8_t my_bingo;
    srand(getpid());
    life = rand() % LIFE_SPAN + 1;
    my_bingo = (uint8_t)(rand() % MAX_NUM);
    while (life--)
    {
        if (TEMP_FAILURE_RETRY(mq_receive(pout, (char *)&ni, 1, NULL)) < 1)
            ERR("mq_receive");
        printf("[%d] Received %d\n", getpid(), ni);
        if (my_bingo == ni)
        {
            if (TEMP_FAILURE_RETRY(mq_send(pin, ( const char *)&my_bingo, 1, 1)))
                ERR("mq_send");
            return;
        }
    }
    if (TEMP_FAILURE_RETRY(mq_send(pin, (const char *)&n, 1, 0)))
        ERR("mq_send");
}

void parent_work(mqd_t pout)
{
    srand(getpid());
    uint8_t ni;
    while (1)
    {
        if(should_return ==1) 
        {
            printf("parent terminating\n");
            return;
        }    
        ni = (uint8_t)(rand() % MAX_NUM);
        if (mq_send(pout, (const char *)&ni, 1, 0))
            ERR("mq_send");
        sleep(1);
    }
}

void create_children(int n, mqd_t pin, mqd_t pout)
{
    while (n-- > 0)
    {
        switch (fork())
        {
            case 0:
                child_work(n, pin, pout);
                exit(EXIT_SUCCESS);
            case -1:
                perror("Fork:");
                exit(EXIT_FAILURE);
        }
    }
            
}

void usage(void)
{
    fprintf(stderr, "USAGE: signals n k p l\n");
    fprintf(stderr, "100 >n > 0 - number of children\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    int n;
    if (argc != 2)
        usage();
    n = atoi(argv[1]);
    if (n <= 0 || n >= 100)
        usage();

    mqd_t pin, pout;
    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 1;

 mq_unlink("/bingo_in");
 mq_unlink("/bingo_out");


    if ((pin = TEMP_FAILURE_RETRY(mq_open("/bingo_in", O_RDWR | O_NONBLOCK | O_CREAT, 0600, &attr))) == (mqd_t)-1)
        ERR("mq open in");
    if ((pout = TEMP_FAILURE_RETRY(mq_open("/bingo_out", O_RDWR | O_CREAT, 0600, &attr))) == (mqd_t)-1)
        ERR("mq open out");

    sethandler(sigchld_handler, SIGCHLD);

    struct sigevent not;
    memset(&not, 0, sizeof(not));
    not.sigev_notify = SIGEV_THREAD;
    not.sigev_notify_function = thread_routine;
    not.sigev_notify_attributes = NULL; // Thread creation attributes
    not.sigev_value.sival_ptr = &pin; // Thread routine argument
    if (mq_notify(pin, &not) < 0) {
        perror("mq_notify()");
        return 1;
    }
    num_children = n;
    create_children(n, pin, pout);
    parent_work(pout);

    mq_close(pin);
    mq_close(pout);
    if (mq_unlink("/bingo_in"))
        ERR("mq unlink");
    if (mq_unlink("/bingo_out"))
        ERR("mq unlink");
    return EXIT_SUCCESS;
}

