#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>

#include "epollinterface.h"
#include "netutils.h"


#define MAX_LISTEN_BACKLOG 1
#define BUFFER_SIZE 4096


struct client_socket_event_data {
    int backend_socket_fd;
};



int handle_client_socket_event(struct epoll_event_handler_data *self, uint32_t events) {
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



struct backend_socket_event_data {
    int client_socket_fd;
};



int handle_backend_socket_event(struct epoll_event_handler_data *self, uint32_t events) {
    struct backend_socket_event_data *closure;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    closure = (struct backend_socket_event_data *) self->closure;

    if ((events & EPOLLERR) | (events & EPOLLHUP) | (events & EPOLLRDHUP)) {
        close(self->fd);
        close(closure->client_socket_fd);
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
            close(closure->client_socket_fd);
            free(closure);
            return 1;
        }

        write(closure->client_socket_fd, buffer, bytes_read);
    }

    return 0;
}



void handle_client_connection(int epoll_fd,
                              int client_socket_fd, 
                              char *backend_host, 
                              char *backend_port_str) 
{

    struct addrinfo hints;
    struct addrinfo *addrs;
    struct addrinfo *addrs_iter;
    int getaddrinfo_error;

    int backend_socket_fd;

    struct epoll_event client_socket_event;
    struct client_socket_event_data *client_socket_event_closure;
    struct epoll_event_handler_data *client_socket_event_handler;

    struct epoll_event backend_socket_event;
    struct backend_socket_event_data *backend_socket_event_closure;
    struct epoll_event_handler_data *backend_socket_event_handler;



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

    make_socket_non_blocking(client_socket_fd);
    client_socket_event_closure = malloc(sizeof(struct client_socket_event_data));
    client_socket_event_closure->backend_socket_fd = backend_socket_fd;

    client_socket_event_handler = malloc(sizeof(struct epoll_event_handler_data));
    client_socket_event_handler->fd = client_socket_fd;
    client_socket_event_handler->handle = handle_client_socket_event;
    client_socket_event_handler->closure = client_socket_event_closure;

    client_socket_event.data.ptr = client_socket_event_handler;
    client_socket_event.events = EPOLLIN | EPOLLRDHUP;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket_fd, &client_socket_event) == -1) {
        perror("Couldn't register client socket with epoll");
        exit(-1);
    }


    make_socket_non_blocking(backend_socket_fd);
    backend_socket_event_closure = malloc(sizeof(struct backend_socket_event_data));
    backend_socket_event_closure->client_socket_fd = client_socket_fd;

    backend_socket_event_handler = malloc(sizeof(struct epoll_event_handler_data));
    backend_socket_event_handler->fd = backend_socket_fd;
    backend_socket_event_handler->handle = handle_backend_socket_event;
    backend_socket_event_handler->closure = backend_socket_event_closure;

    backend_socket_event.data.ptr = backend_socket_event_handler;
    backend_socket_event.events = EPOLLIN | EPOLLRDHUP;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, backend_socket_fd, &backend_socket_event) == -1) {
        perror("Couldn't register backend socket with epoll");
        exit(-1);
    }

}


int create_and_bind(char *server_port_str) {
    int server_socket_fd;

    struct addrinfo hints;
    struct addrinfo *addrs;
    struct addrinfo *addr_iter;
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


struct server_socket_event_data {
    int epoll_fd;
    char *backend_addr;
    char *backend_port_str;
};



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



int main(int argc, char *argv[]) {
    char *server_port_str;
    char *backend_addr;
    char *backend_port_str;

    int epoll_fd;
    struct epoll_event server_socket_event;

    int server_socket_fd;
    struct epoll_event_handler_data *server_socket_event_handler;
    struct server_socket_event_data *server_socket_event_closure;


    if (argc != 4) {
        fprintf(stderr, 
                "Usage: %s <server_port> <backend_addr> <backend_port>\n", 
                argv[0]);
        exit(1);
    }
    server_port_str = argv[1];
    backend_addr = argv[2];
    backend_port_str = argv[3];

    server_socket_fd = create_and_bind(server_port_str);
    make_socket_non_blocking(server_socket_fd);

    listen(server_socket_fd, MAX_LISTEN_BACKLOG);
    printf("Started.  Listening on port %s.\n", server_port_str);

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Couldn't create epoll FD");
        exit(1);
    }

    server_socket_event_closure = malloc(sizeof(struct server_socket_event_data));
    server_socket_event_closure->epoll_fd = epoll_fd;
    server_socket_event_closure->backend_addr = backend_addr;
    server_socket_event_closure->backend_port_str = backend_port_str;

    server_socket_event_handler = malloc(sizeof(struct epoll_event_handler_data));
    server_socket_event_handler->fd = server_socket_fd;
    server_socket_event_handler->handle = handle_server_socket_event;
    server_socket_event_handler->closure = server_socket_event_closure;

    server_socket_event.data.ptr = server_socket_event_handler;
    server_socket_event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket_fd, &server_socket_event) == -1) {
        perror("Couldn't register server socket with epoll");
        exit(-1);
    }

    do_reactor_loop(epoll_fd);

}

