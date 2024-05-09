#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define MAX_CLIENTS 1
#define MAX_EVENTS 10

#define ERR(source) (perror(source),                                 \
                     fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
                     exit(EXIT_FAILURE))

void usage(char *name) { fprintf(stderr, "USAGE: %s host port\n", name); }

int make_tcp_socket()
{
    int sock;
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        ERR("socket");
    return sock;
}

int bind_tcp_socket(uint16_t port, int backlog_size)
{
    struct sockaddr_in addr;
    int socketfd, t = 1;
    socketfd = make_tcp_socket();
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)))
        ERR("setsockopt");
    if (bind(socketfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        ERR("bind");
    if (listen(socketfd, backlog_size) < 0)
        ERR("listen");
    return socketfd;
}

int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    int server_socketfd;
    if ((server_socketfd = bind_tcp_socket(atoi(argv[2]), MAX_CLIENTS)) < 0)
    {
        ERR("bind");
    }

    printf("Server is listening on port %s...\n", argv[2]);

    int epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        ERR("epoll_create1");
    }

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_socketfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_socketfd, &ev) == -1) {
        ERR("epoll_ctl: server_socketfd");
    }
    while (1)
    {
       int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            ERR("epoll_wait");
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == server_socketfd) {
                int client_socketfd = accept(server_socketfd, (struct sockaddr *)&client_addr, &addr_len);
                if (client_socketfd == -1) {
                    ERR("accept");
                } else {
                    printf("A client has connected.\n");
                    ev.events = EPOLLIN;
                    ev.data.fd = client_socketfd;
                    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_socketfd, &ev) == -1) {
                        ERR("epoll_ctl: client_socketfd");
                    }
                }
            } else {
                // Handle client connection here...
            }
        }
    }

    close(server_socketfd);

    return 0;
}