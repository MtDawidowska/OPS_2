#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define MAX_CLIENTS 1
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

void string_invert_case(char *s){ // invert cases of letters
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

void string_swap_spaces(char *s){ // space to underscore and vice versa
    for (int i = 0; s[i]; i++)
    {
        if (s[i] == ' ')
            s[i] = '_';
        else if (s[i] == '_')
            s[i] = ' ';
    }
}

void string_modify(char *s){ // both of the above
    string_invert_case(s);
    string_swap_spaces(s);
}

int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    int client_socketfd;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    char buffer[BUFFER_SIZE];

    int server_socketfd;
    if ((server_socketfd = bind_tcp_socket(atoi(argv[2]), MAX_CLIENTS)) < 0)
    {
        ERR("bind");
    }

    printf("Server is listening on port %s...\n", argv[2]);

    FILE *file = fopen("output.txt", "w");
    if (file == NULL)
    {
        ERR("fopen");
    }

    while (1)
    {
        if ((client_socketfd = accept(server_socketfd, (struct sockaddr *)&client_addr, &addr_len)) == -1)
        {
            ERR("accept");
        }
        else
        {
            printf("A client has connected.\n");
            while (read(client_socketfd, buffer, sizeof(buffer) - 1) > 0)
            {
                write(fileno(file), buffer, strlen(buffer));
            }
            memset(buffer, 0, sizeof(buffer));
            close(client_socketfd);
        }
    }

    fclose(file);
    close(server_socketfd);

    return 0;
}