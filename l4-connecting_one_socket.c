#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 1

#define ERR(source) (perror(source),\
                     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))

void usage(char *name) { fprintf(stderr, "USAGE: %s host port\n", name); }

int make_tcp_socket() {
    int sock;
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        ERR("socket");
    return sock;
}

int bind_tcp_socket(uint16_t port, int backlog_size) {
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

int main(int argc, char *argv[]) {

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

    while (1) {
        if (accept(server_socketfd, (struct sockaddr *)&client_addr, &addr_len) == -1) {
            perror("accept");
        } else {
            printf("A client has connected.\n");
        }
    }

    close(server_socketfd);

    return 0;
}