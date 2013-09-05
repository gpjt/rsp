#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>

#include "netutils.h"
#include "epollinterface.h"
#include "backend_socket.h"
#include "client_socket.h"


#define BUFFER_SIZE 4096


struct backend_socket_event_data {
    struct epoll_event_handler* client_handler;
};


void handle_backend_socket_event(struct epoll_event_handler* self, uint32_t events)
{
    struct backend_socket_event_data* closure = (struct backend_socket_event_data*) self->closure;

    char buffer[BUFFER_SIZE];
    int bytes_read;

    if (events & EPOLLIN) {
        bytes_read = read(self->fd, buffer, BUFFER_SIZE);
        if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return;
        }

        if (bytes_read == 0 || bytes_read == -1) {
            close_client_socket(closure->client_handler);
            close_backend_socket(self);
            return;
        }

        write(closure->client_handler->fd, buffer, bytes_read);
    }

    if ((events & EPOLLERR) | (events & EPOLLHUP) | (events & EPOLLRDHUP)) {
        close_client_socket(closure->client_handler);
        close_backend_socket(self);
        return;
    }

    return;
}



void close_backend_socket(struct epoll_event_handler* self)
{
    close(self->fd);
    free(self->closure);
    free(self);
}



struct epoll_event_handler* create_backend_socket_handler(int backend_socket_fd, 
                                                          struct epoll_event_handler* client_handler)
{
    make_socket_non_blocking(backend_socket_fd);

    struct backend_socket_event_data* closure = malloc(sizeof(struct backend_socket_event_data));
    closure->client_handler = client_handler;

    struct epoll_event_handler* result = malloc(sizeof(struct epoll_event_handler));
    result->fd = backend_socket_fd;
    result->handle = handle_backend_socket_event;
    result->closure = closure;

    return result;
}

