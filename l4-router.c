#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <errno.h>
#define MAX_CLIENTS 8
#define MAX_EVENTS 10

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
int clients[MAX_CLIENTS];

void usage(char *name) { fprintf(stderr, "USAGE: %s host port\n", name); }

int add_new_client(int sfd)
{
    int nfd;
    if ((nfd = TEMP_FAILURE_RETRY(accept(sfd, NULL, NULL))) < 0)
    {
        if (EAGAIN == errno || EWOULDBLOCK == errno)
            return -1;
        ERR("accept");
    }

    char address;
    if (bulk_read(nfd, &address, 1) != 1)
        ERR("read");

    address -= '0';
    printf("Client %d connected\n", address);

    if (address < 1 || address > MAX_CLIENTS || clients[address - 1] != 0)
    {
        char *msg = "Wrong address";
        bulk_write(nfd, msg, strlen(msg));
        close(nfd);
        return -1;
    }

    clients[address - 1] = nfd;
    char send_address = address + '0';
    bulk_write(nfd, &send_address, 1);
    return nfd;
}

void do_server(int server_sockfd)
{
    int epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        ERR("epoll_create1");
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = server_sockfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_sockfd, &event) == -1)
    {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    clients[0] = server_sockfd;

    struct epoll_event events[MAX_EVENTS];

    while (1)
    {
        int num_events = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (num_events == -1)
        {
            ERR("epoll_wait");
        }

        for (int i = 0; i < num_events; i++)
        {
            if (events[i].data.fd == server_sockfd)
            { // incoming connection
                int client_sockfd = add_new_client(server_sockfd);
                if (client_sockfd < 0)
                {
                    ERR("accept");
                }

                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_sockfd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_sockfd, &event) == -1)
                {
                    ERR("epoll_ctl");
                }
            }
            else
            { // incoming data

                char buffer[1024];
                ssize_t size = read(events[i].data.fd, buffer, sizeof(buffer) - 1);
                if (size > 0)
                {
                    printf("%s\n", buffer);
                }
                // 
                // else if (size == 0)
                // {
                //     printf("Client disconnected\n");
                //     close(events[i].data.fd);
                //     for (int j = 0; j < MAX_CLIENTS; j++)
                //     {
                //         if (clients[j] == events[i].data.fd)
                //         {
                //             clients[j] = 0;
                //             break;
                //         }
                //     }
                // }

                else if (size < -1)
                {
                    if (errno != EAGAIN)
                    {
                        perror("read");
                    }
                    continue;
                }
            memset(buffer, 0, sizeof(buffer));
            }
        }
    }
}


int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_sockfd;
    if ((server_sockfd = bind_tcp_socket(atoi(argv[2]), MAX_CLIENTS)) < 0)
    {
        ERR("bind");
    }

    do_server(server_sockfd);

    return 0;
}