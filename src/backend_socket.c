#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>

#include "netutils.h"
#include "epollinterface.h"
#include "backend_socket.h"


#define BUFFER_SIZE 4096


struct backend_socket_event_data {
    int client_socket_fd;
};


int handle_backend_socket_event(struct epoll_event_handler* self, uint32_t events) {
    struct backend_socket_event_data* closure;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    closure = (struct backend_socket_event_data*) self->closure;

    if (events & EPOLLIN) {
        bytes_read = read(self->fd, buffer, BUFFER_SIZE);
        if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return 0;
        }

        if (bytes_read == 0 || bytes_read == -1) {
            close(self->fd);
            close(closure->client_socket_fd);
            free(closure);
            return 1;
        }

        write(closure->client_socket_fd, buffer, bytes_read);
    }

    if ((events & EPOLLERR) | (events & EPOLLHUP) | (events & EPOLLRDHUP)) {
        close(self->fd);
        close(closure->client_socket_fd);
        free(closure);
        return 1;
    }

    return 0;
}


struct epoll_event_handler* create_backend_socket_handler(int backend_socket_fd, int client_socket_fd) {
    struct backend_socket_event_data* closure;
    struct epoll_event_handler* result;

    make_socket_non_blocking(backend_socket_fd);

    closure = malloc(sizeof(struct backend_socket_event_data));
    closure->client_socket_fd = client_socket_fd;

    result = malloc(sizeof(struct epoll_event_handler));
    result->fd = backend_socket_fd;
    result->handle = handle_backend_socket_event;
    result->closure = closure;

    return result;

}

