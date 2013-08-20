#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>

#include "epollinterface.h"
#include "client_socket.h"

#define BUFFER_SIZE 4096

struct client_socket_event_data {
    int backend_socket_fd;
};


int handle_client_socket_event(struct epoll_event_handler *self, uint32_t events) {
    struct client_socket_event_data *closure;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    closure = (struct client_socket_event_data *) self->closure;

    if ((events & EPOLLERR) | (events & EPOLLHUP) | (events & EPOLLRDHUP)) {
        close(self->fd);
        close(closure->backend_socket_fd);
        free(closure);
        return 1;
    }

    if (events & EPOLLIN) {
        bytes_read = read(self->fd, buffer, BUFFER_SIZE);
        if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return 0;
        }

        if (bytes_read == 0 || bytes_read == -1) {
            close(self->fd);
            close(closure->backend_socket_fd);
            free(closure);
            return 1;
        }

        write(closure->backend_socket_fd, buffer, bytes_read);
    }

    return 0;
}


struct epoll_event_handler *create_client_socket_handler(int client_socket_fd, int backend_socket_fd) {
    struct client_socket_event_data *closure;
    struct epoll_event_handler *result;

    make_socket_non_blocking(client_socket_fd);

    closure = malloc(sizeof(struct client_socket_event_data));
    closure->backend_socket_fd = backend_socket_fd;

    result = malloc(sizeof(struct epoll_event_handler));
    result->fd = client_socket_fd;
    result->handle = handle_client_socket_event;
    result->closure = closure;

}
