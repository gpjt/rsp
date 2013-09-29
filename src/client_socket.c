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
    struct epoll_event_handler* backend_handler;
};


void handle_client_socket_event(struct epoll_event_handler* self, uint32_t events)
{
    struct client_socket_event_data* closure = (struct client_socket_event_data* ) self->closure;

    char buffer[BUFFER_SIZE];
    int bytes_read;

    if (events & EPOLLIN) {
        while ((bytes_read = read(self->fd, buffer, BUFFER_SIZE)) != -1 && bytes_read != 0) {
            if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                return;
            }

            if (bytes_read == 0 || bytes_read == -1) {
                close_backend_socket(closure->backend_handler);
                close_client_socket(self);
                return;
            }

            write(closure->backend_handler->fd, buffer, bytes_read);
        }
    }

    if ((events & EPOLLERR) | (events & EPOLLHUP) | (events & EPOLLRDHUP)) {
        close_backend_socket(closure->backend_handler);
        close_client_socket(self);
        return;
    }

}



void close_client_socket(struct epoll_event_handler* self)
{
    close(self->fd);
    free(self->closure);
    free(self);
}



struct epoll_event_handler* connect_to_backend(struct epoll_event_handler* client_handler,
                                               int epoll_fd,
                                               char* backend_host,
                                               char* backend_port_str)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int getaddrinfo_error;
    struct addrinfo* addrs;
    getaddrinfo_error = getaddrinfo(backend_host, backend_port_str, &hints, &addrs);
    if (getaddrinfo_error != 0) {
        if (getaddrinfo_error == EAI_SYSTEM) {
            fprintf(stderr, "Couldn't find backend: system error: %s\n", strerror(errno));
        } else {
            fprintf(stderr, "Couldn't find backend: %s\n", gai_strerror(getaddrinfo_error));
        }
        exit(1);
    }

    int backend_socket_fd;
    struct addrinfo* addrs_iter;
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

    struct epoll_event_handler* backend_socket_event_handler;
    backend_socket_event_handler = create_backend_socket_handler(backend_socket_fd, client_handler);
    add_epoll_handler(epoll_fd, backend_socket_event_handler, EPOLLIN | EPOLLRDHUP | EPOLLET);

    return backend_socket_event_handler;
}



struct epoll_event_handler* create_client_socket_handler(int client_socket_fd,
                                                         int epoll_fd,
                                                         char* backend_host,
                                                         char* backend_port_str)
{
    
    make_socket_non_blocking(client_socket_fd);

    struct client_socket_event_data* closure = malloc(sizeof(struct client_socket_event_data));

    struct epoll_event_handler* result = malloc(sizeof(struct epoll_event_handler));
    result->fd = client_socket_fd;
    result->handle = handle_client_socket_event;
    result->closure = closure;

    closure->backend_handler = connect_to_backend(result, epoll_fd, backend_host, backend_port_str);

    return result;
}
