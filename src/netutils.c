#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>


int make_socket_non_blocking(int socket_fd) {
    int flags;
    int fcntl_set_result;

    flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("Couldn't get socket flags");
        exit(1);
    }

    flags |= O_NONBLOCK;
    if (fcntl(socket_fd, F_SETFL, flags) == -1) {
        perror("Couldn't set socket flags");
        exit(-1);
    }
}


