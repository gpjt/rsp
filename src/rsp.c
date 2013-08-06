#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int arg, char *argv[]) {
    int server_socket_fd;
    struct sockaddr_in server_socket_addr;

    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd < 0) {
        perror("Could not open socket");
        exit(1);
    }

    bzero((char *) &server_socket_addr, sizeof(server_socket_addr));
    server_socket_addr.sin_family = AF_INET;
    server_socket_addr.sin_port = htons(atoi(argv[1]));
    server_socket_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket_fd, (struct sockaddr *) &server_socket_addr, sizeof(server_socket_addr)) < 0) {
        perror("Could not bind");
        exit(1);
    }
}
