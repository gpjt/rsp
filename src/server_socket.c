#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "epollinterface.h"
#include "server_socket.h"
#include "client_socket.h"
#include "backend_socket.h"

#define MAX_LISTEN_BACKLOG 10


struct server_socket_event_data {
    int epoll_fd;
    char* backend_addr;
    char* backend_port_str;
};


void handle_client_connection(int epoll_fd,
                              int client_socket_fd, 
                              char* backend_host, 
                              char* backend_port_str) 
{

    struct addrinfo hints;
    struct addrinfo* addrs;
    struct addrinfo* addrs_iter;
    int getaddrinfo_error;

    int backend_socket_fd;

    struct client_socket_event_data* client_socket_event_closure;
    struct epoll_event_handler* client_socket_event_handler;

    struct backend_socket_event_data* backend_socket_event_closure;
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

    client_socket_event_handler = create_client_socket_handler(client_socket_fd, backend_socket_fd);
    add_epoll_handler(epoll_fd, client_socket_event_handler, EPOLLIN | EPOLLRDHUP);

    backend_socket_event_handler = create_backend_socket_handler(backend_socket_fd, client_socket_fd);
    add_epoll_handler(epoll_fd, backend_socket_event_handler, EPOLLIN | EPOLLRDHUP);

}



int handle_server_socket_event(struct epoll_event_handler* self, uint32_t events) {
    struct server_socket_event_data* closure;
    int client_socket_fd;

    closure = (struct server_socket_event_data*) self->closure;

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


int create_and_bind(char* server_port_str) {
    int server_socket_fd;

    struct addrinfo hints;
    struct addrinfo* addrs;
    struct addrinfo* addr_iter;
    int getaddrinfo_error;

    int so_reuseaddr;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo_error = getaddrinfo(NULL, server_port_str, &hints, &addrs);
    if (getaddrinfo_error != 0) {
        fprintf(stderr, "Couldn't find local host details: %s\n", gai_strerror(getaddrinfo_error));
        exit(1);
    }

    for (addr_iter = addrs; addr_iter != NULL; addr_iter = addr_iter->ai_next) {
        server_socket_fd = socket(addr_iter->ai_family,
                                  addr_iter->ai_socktype,
                                  addr_iter->ai_protocol);
        if (server_socket_fd == -1) {
            continue;
        }

        so_reuseaddr = 1;
        setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof(so_reuseaddr));

        if (bind(server_socket_fd,
                 addr_iter->ai_addr,
                 addr_iter->ai_addrlen) == 0)
        {
            break;
        }

        close(server_socket_fd);
    }

    if (addr_iter == NULL) {
        fprintf(stderr, "Couldn't bind\n");
        exit(1);
    }

    freeaddrinfo(addrs);

    return server_socket_fd;
}


struct epoll_event_handler* create_server_socket_handler(int epoll_fd, char* server_port_str, char* backend_addr, char* backend_port_str) {
    struct epoll_event_handler* result;
    struct server_socket_event_data* closure;

    int server_socket_fd;

    server_socket_fd = create_and_bind(server_port_str);
    make_socket_non_blocking(server_socket_fd);

    listen(server_socket_fd, MAX_LISTEN_BACKLOG);

    closure = malloc(sizeof(struct server_socket_event_data));
    closure->epoll_fd = epoll_fd;
    closure->backend_addr = backend_addr;
    closure->backend_port_str = backend_port_str;

    result = malloc(sizeof(struct epoll_event_handler));
    result->fd = server_socket_fd;
    result->handle = handle_server_socket_event;
    result->closure = closure;

    return result;
}


