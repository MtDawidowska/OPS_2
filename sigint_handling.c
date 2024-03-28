#include <errno.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>


volatile sig_atomic_t should_exit = 0; 
void sigint_handler(int signum) {
    should_exit = 1; 
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    for (int i = 1; i < NSIG; i++) {
        if (i != SIGINT) {
            signal(i, SIG_IGN);
        }
    }

    while (!should_exit) {
        printf("*\n");
        sleep(1);
    }

    printf("Received SIGINT. Exiting...\n");

    return 0;
}
