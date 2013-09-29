#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <string.h>

#include "epollinterface.h"


void print_stuff(struct epoll_event_handler* self, uint32_t events)
{
    printf("Got an event!\n");
}


int main(int argc, char* argv[]) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int getaddrinfo_error;
    struct addrinfo* addrs;
    getaddrinfo_error = getaddrinfo("www.google.com", "80", &hints, &addrs);
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

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Couldn't create epoll FD");
        exit(1);
    }

    struct epoll_event_handler* backend_socket_event_handler = malloc(sizeof(struct epoll_event_handler));
    backend_socket_event_handler->fd = backend_socket_fd;
    backend_socket_event_handler->handle = print_stuff;
    add_epoll_handler(epoll_fd, backend_socket_event_handler, EPOLLOUT | EPOLLET);

    do_reactor_loop(epoll_fd);
}
