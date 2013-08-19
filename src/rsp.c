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

