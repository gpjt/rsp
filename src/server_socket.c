#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include "epollinterface.h"
#include "server_socket.h"


int handle_server_socket_event(struct epoll_event_handler_data *self, uint32_t events) {
    struct server_socket_event_data *closure;
    int client_socket_fd;

    closure = (struct server_socket_event_data *) self->closure;

    while (1) {
        client_socket_fd = accept(self->fd, NULL, NULL);
        if (client_socket_fd == -1) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                break;
            } else {
                perror("Could not accept");
                exit(1);
            }
        }

        handle_client_connection(closure->epoll_fd, client_socket_fd, closure->backend_addr, closure->backend_port_str);
    }

    return 0;
}


struct epoll_event_handler_data *create_server_socket_handler(int server_socket_fd, int epoll_fd, char *backend_addr, char *backend_port_str) {
    struct epoll_event_handler_data *result;
    struct server_socket_event_data *closure;

    closure = malloc(sizeof(struct server_socket_event_data));
    closure->epoll_fd = epoll_fd;
    closure->backend_addr = backend_addr;
    closure->backend_port_str = backend_port_str;

    result = malloc(sizeof(struct epoll_event_handler_data));
    result->fd = server_socket_fd;
    result->handle = handle_server_socket_event;
    result->closure = closure;

    return result;
}


