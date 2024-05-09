#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/epoll.h>
#include <time.h>

#define MAX_EVENTS 10
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

#define ERR(source) (perror(source),                                 \
                     fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
                     exit(EXIT_FAILURE))

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression)             \
    (__extension__({                               \
        long int __result;                         \
        do                                         \
            __result = (long int)(expression);     \
        while (__result == -1L && errno == EINTR); \
        __result;                                  \
    }))
#endif

void usage(char *name) { fprintf(stderr, "USAGE: %s host port\n", name); }

ssize_t bulk_read(int fd, char *buf, size_t count)
{
    int c;
    size_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (0 == c)
            return len;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
    int c;
    size_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

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

void string_invert_case(char *s)
{ // invert cases of letters
    int j = 0;
    for (int i = 0; s[i]; i++)
    {
        if (!isdigit(s[i]))
        {
            if (isupper(s[i]))
                s[j] = tolower(s[i]);
            else if (islower(s[i]))
                s[j] = toupper(s[i]);
            else
                s[j] = s[i];
            j++;
        }
    }
    s[j] = '\0';
}

void string_swap_spaces(char *s)
{ // space to underscore and vice versa
    for (int i = 0; s[i]; i++)
    {
        if (s[i] == ' ')
            s[i] = '_';
        else if (s[i] == '_')
            s[i] = ' ';
    }
}

void string_modify(char *s, char mode)
{
    switch (mode)
    {
    case '1':
        string_invert_case(s);
        break;
    case '2':
        string_swap_spaces(s);
        break;
    case '3':
        string_invert_case(s);
        string_swap_spaces(s);
        break;
    default:
        break;
    }
}

char* get_timestamp()
{
    static char timestamp[6];
    time_t current_time = time(NULL);

    if (current_time == (time_t)-1) {
        ERR( "time(NULL)");
    }

    char* c_time_string = ctime(&current_time);
    if (c_time_string == NULL) {
        ERR("ctime");
    }

    strncpy(timestamp, &c_time_string[11], 5);
    timestamp[5] = '\0';  
    return timestamp;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    char *log_file_path = argv[1];
    int active_clients = 0;

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    int server_socketfd;
    if ((server_socketfd = bind_tcp_socket(atoi(argv[3]), MAX_CLIENTS)) < 0)
    {
        ERR("bind");
    }

    printf("Server is listening on port %s...\n", argv[3]);

    FILE *file = fopen(log_file_path, "w");
    if (file == NULL)
    {
        ERR("fopen");
    }

    int epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        ERR("epoll_create1");
    }

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_socketfd;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_socketfd, &ev) == -1)
    {
        ERR("epoll_ctl: server_socketfd");
    }

    while (1)
    {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1)
        {
            ERR("epoll_wait");
        }

        for (int n = 0; n < nfds; ++n)
        {
            if (events[n].data.fd == server_socketfd)
            {
                if (active_clients >= MAX_CLIENTS) //reject
                {
                    int reject_socketfd = accept(server_socketfd, (struct sockaddr *)&client_addr, &addr_len);
                    if (reject_socketfd != -1)
                    {
                        char *msg = "Error 23: Too many active clients\n";
                        write(reject_socketfd, msg, strlen(msg));
                        close(reject_socketfd);
                    }
                }
                else
                {
                    int client_socketfd = accept(server_socketfd, (struct sockaddr *)&client_addr, &addr_len);
                    if (client_socketfd == -1)
                    {
                        ERR("accept");
                    }
                    else
                    {
                        printf("A client has connected.\n");
                        ev.events = EPOLLIN;
                        ev.data.fd = client_socketfd;
                        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_socketfd, &ev) == -1)
                        {
                            ERR("epoll_ctl: client_socketfd");
                        }
                        active_clients++;
                    }
                }
            }
            else
            {
                // Handle client connection here...
                ssize_t count = read(events[n].data.fd, buffer, sizeof(buffer) - 1);
                if (count > 0)
                {
                    char mode = buffer[0];
                    if (mode < '1' || mode > '3')
                    {
                        char *msg = "Error 17: Wrong filter mode";
                        write(events[n].data.fd, msg, strlen(msg));
                        continue;
                    }
                    string_modify(buffer + 1, mode);
                   
                    char* local_time = get_timestamp();
                    fprintf(file, "[%s] %s", local_time, buffer + 1);
                    fflush(file);
                    memset(buffer, 0, sizeof(buffer));
                }
                else if (count == 0 || (count == -1 && errno != EAGAIN))
                {
                    close(events[n].data.fd);
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, NULL);
                    active_clients--;
                }
            }
        }
    }

    fclose(file);
    close(server_socketfd);

    return 0;
}