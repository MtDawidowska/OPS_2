#include "l4_common.h"

#define MSG_SIZE_SEND 5

void usage(char *name) { fprintf(stderr, "USAGE: %s port\n", name); }

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    printf("PID = %d\n", getpid());
    int fd;
    fd = connect_tcp_socket(argv[1], argv[2]);
    
    char send_buffer[MSG_SIZE_SEND];
    sprintf(send_buffer, "%d", getpid());
    if (bulk_write(fd, (char *)send_buffer, sizeof(send_buffer)) < 0)
        ERR("write:");
        
    int16_t receive_buffer;
    if (read(fd, &receive_buffer, sizeof(int16_t)) < sizeof(int16_t))
        ERR("read:");
    
    receive_buffer = ntohs(receive_buffer);
    printf("SUM = %d\n",receive_buffer);

    if (TEMP_FAILURE_RETRY(close(fd)) < 0)
        ERR("close");
    return EXIT_SUCCESS;
}
