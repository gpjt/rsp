#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <string.h>


#include "netutils.h"
#include "epollinterface.h"
#include "backend_socket.h"
#include "client_socket.h"

#define BUFFER_SIZE 4096

struct client_socket_event_data {
    int backend_socket_fd;
};


int handle_client_socket_event(struct epoll_event_handler* self, uint32_t events) {
    struct client_socket_event_data* closure;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    closure = (struct client_socket_event_data* ) self->closure;

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

    if ((events & EPOLLERR) | (events & EPOLLHUP) | (events & EPOLLRDHUP)) {
        close(self->fd);
        close(closure->backend_socket_fd);
        free(closure);
        return 1;
    }

    return 0;
}


int connect_to_backend(int client_socket_fd, int epoll_fd, char* backend_host, char* backend_port_str) {
    struct addrinfo hints;
    struct addrinfo* addrs;
    struct addrinfo* addrs_iter;
    int getaddrinfo_error;

    int backend_socket_fd;

    struct epoll_event_handler* backend_socket_event_handler;



    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo_error = getaddrinfo(backend_host, backend_port_str, &hints, &addrs);
    if (getaddrinfo_error != 0) {
        fprintf(stderr, "Couldn't find backend: %s\n", gai_strerror(getaddrinfo_error));
        exit(1);
    }

    for (addrs_iter = addrs;
         addrs_iter != NULL;
         addrs_iter = addrs_iter->ai_next)
    {
        backend_socket_fd = socket(addrs_iter->ai_family,
                                   addrs_iter->ai_socktype,
                                   addrs_iter->ai_protocol);
        if (backend_socket_fd == -1) {
            continue;
        }

        if (connect(backend_socket_fd,
                    addrs_iter->ai_addr,
                    addrs_iter->ai_addrlen) != -1) {
            break;
        }

        close(backend_socket_fd);
    }

    if (addrs_iter == NULL) {
        fprintf(stderr, "Couldn't connect to backend");
        exit(1);
    }

    freeaddrinfo(addrs);

    backend_socket_event_handler = create_backend_socket_handler(backend_socket_fd, client_socket_fd);
    add_epoll_handler(epoll_fd, backend_socket_event_handler, EPOLLIN | EPOLLRDHUP);

    return backend_socket_fd;
}


struct epoll_event_handler* create_client_socket_handler(int client_socket_fd, int epoll_fd, char* backend_host, char* backend_port_str) {
    struct client_socket_event_data* closure;
    struct epoll_event_handler* result;
    
    make_socket_non_blocking(client_socket_fd);

    closure = malloc(sizeof(struct client_socket_event_data));
    closure->backend_socket_fd = connect_to_backend(client_socket_fd, epoll_fd, backend_host, backend_port_str);

    result = malloc(sizeof(struct epoll_event_handler));
    result->fd = client_socket_fd;
    result->handle = handle_client_socket_event;
    result->closure = closure;

    return result;
}
