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
#include "server_socket.h"
#include "client_socket.h"
#include "backend_socket.h"



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

    struct client_socket_event_data *client_socket_event_closure;
    struct epoll_event_handler_data *client_socket_event_handler;

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

    add_epoll_handler(epoll_fd, client_socket_event_handler, EPOLLIN | EPOLLRDHUP);

    make_socket_non_blocking(backend_socket_fd);
    backend_socket_event_closure = malloc(sizeof(struct backend_socket_event_data));
    backend_socket_event_closure->client_socket_fd = client_socket_fd;

    backend_socket_event_handler = malloc(sizeof(struct epoll_event_handler_data));
    backend_socket_event_handler->fd = backend_socket_fd;
    backend_socket_event_handler->handle = handle_backend_socket_event;
    backend_socket_event_handler->closure = backend_socket_event_closure;

    add_epoll_handler(epoll_fd, backend_socket_event_handler, EPOLLIN | EPOLLRDHUP);

}


int main(int argc, char *argv[]) {
    char *server_port_str;
    char *backend_addr;
    char *backend_port_str;

    int epoll_fd;
    struct epoll_event_handler_data *server_socket_event_handler;


    if (argc != 4) {
        fprintf(stderr, 
                "Usage: %s <server_port> <backend_addr> <backend_port>\n", 
                argv[0]);
        exit(1);
    }
    server_port_str = argv[1];
    backend_addr = argv[2];
    backend_port_str = argv[3];

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Couldn't create epoll FD");
        exit(1);
    }

    server_socket_event_handler = create_server_socket_handler(epoll_fd, 
                                                               server_port_str,
                                                               backend_addr,
                                                               backend_port_str);

    add_epoll_handler(epoll_fd, server_socket_event_handler, EPOLLIN);

    printf("Started.  Listening on port %s.\n", server_port_str);
    do_reactor_loop(epoll_fd);

}

