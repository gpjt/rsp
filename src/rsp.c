#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

int main(int arg, char *argv[]) {
    int server_socket_fd;

    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd < 0) {
        perror("Could not open socket");
        exit(1);
    }
}
