#include "l4_common.h"
#include <pthread.h>

#define BACKLOG 3
#define MAX_EVENTS 16
#define MSG_SIZE_SEND 5

volatile sig_atomic_t do_work = 1;

void sigint_handler(int sig) { do_work = 0; }

void usage(char *name) { fprintf(stderr, "USAGE: %s socket port\n", name); }

int16_t sum_digits(char *number)
{
    int16_t sum = 0;
    for (int i = 0; i < 5; i++)
    {
        if (number[i] >= '0' && number[i] <= '9')
        {
            sum += number[i] - '0';
        }
    }
    return sum;
}

int main(int argc, char **argv)
{
    int highest_sum = 0;
    int tcp_socket_fd;
    if (argc != 2)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("Seting SIGPIPE:");
    if (sethandler(sigint_handler, SIGINT))
        ERR("Seting SIGINT:");

    tcp_socket_fd = bind_tcp_socket((uint16_t)atoi(argv[1]), BACKLOG); // makes and binds
    listen(tcp_socket_fd, 5);

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        ERR("epoll_create1");
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = tcp_socket_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tcp_socket_fd, &event) == -1)
    {
        ERR("epoll_ctl");
    }
    struct epoll_event events[MAX_EVENTS];
    while (do_work)
    {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1)
        {
            if (errno == EINTR)
                continue;
            else
                ERR("epoll_wait");
        }

        for (int i = 0; i < num_events; i++)
        {
            if (events[i].data.fd == tcp_socket_fd)
            {
                int client_socket = add_new_client(tcp_socket_fd);
                if (client_socket < 0)
                {
                    ERR("add_new_client");
                }

                event.events = EPOLLIN;
                event.data.fd = client_socket;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1)
                {
                    ERR("epoll_ctl");
                }
            }
            else
            {
                int client_socket = events[i].data.fd;

                char receive_buffer[MSG_SIZE_SEND];
                read(client_socket, &receive_buffer, sizeof(receive_buffer));

                int16_t send_buffer = sum_digits(receive_buffer);

                if (send_buffer > highest_sum)
                    highest_sum = send_buffer;

                send_buffer = htons(send_buffer);
                write(client_socket, &send_buffer, sizeof(send_buffer));

                if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, NULL) == -1)
                {
                    ERR("epoll_ctl");
                }

                close(client_socket);
            }
        }
    }

    printf("Highest sum of digits: %d\n", highest_sum);

    return EXIT_SUCCESS;
}
