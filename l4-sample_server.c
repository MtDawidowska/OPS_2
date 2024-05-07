#include "l4_common.h"

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
        {                           // Check if the character is a digit
            sum += number[i] - '0'; // Convert character to integer and add to sum
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

    int client_socket;
    if ((client_socket = add_new_client(tcp_socket_fd)) < 0)
        ERR("add_new_client");

    while (do_work)
    {
        char receive_buffer[MSG_SIZE_SEND];
        int flags = fcntl(client_socket, F_GETFL, 0);
        fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);
        read(client_socket, &receive_buffer, sizeof(receive_buffer));

        int16_t send_buffer = sum_digits(receive_buffer);

        if (send_buffer > highest_sum)
            highest_sum = send_buffer;

        send_buffer = htons(send_buffer);
        write(client_socket, &send_buffer, sizeof(send_buffer));
    }
    printf("Highest sum of digits: %d\n", highest_sum);
    close(client_socket);

    return EXIT_SUCCESS;
}
