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

#define MAX_EPOLL_EVENTS 10
#define MAX_LISTEN_BACKLOG 1
#define BUFFER_SIZE 4096


void handle_client_connection(int client_socket_fd, 
                              char *backend_host, 
                              char *backend_port_str) 
{

    struct addrinfo hints;
    struct addrinfo *addrs;
    struct addrinfo *addrs_iter;
    int getaddrinfo_error;

    int backend_socket_fd;

    char buffer[BUFFER_SIZE];
    int bytes_read;

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

    bytes_read = read(client_socket_fd, buffer, BUFFER_SIZE);
    write(backend_socket_fd, buffer, bytes_read);

    while (bytes_read = read(backend_socket_fd, buffer, BUFFER_SIZE)) {
        write(client_socket_fd, buffer, bytes_read);
    }

    close(client_socket_fd);
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


int make_socket_non_blocking(int socket_fd) {
    int flags;
    int fcntl_set_result;

    flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("Couldn't get socket flags");
        exit(1);
    }

    flags |= O_NONBLOCK;
    if (fcntl(socket_fd, F_SETFL, flags) == -1) {
        perror("Couldn't set socket flags");
        exit(-1);
    }
}



struct server_socket_event_closure {
    char *backend_addr;
    char *backend_port_str;
};



void handle_server_socket_event(int server_socket_fd, struct server_socket_event_closure *closure) {
    int client_socket_fd;

    while (1) {
        client_socket_fd = accept(server_socket_fd, NULL, NULL);
        if (client_socket_fd == -1) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                break;
            } else {
                perror("Could not accept");
                exit(1);
            }
        }

        handle_client_connection(client_socket_fd, closure->backend_addr, closure->backend_port_str);
    }
}



int main(int argc, char *argv[]) {
    char *server_port_str;
    char *backend_addr;
    char *backend_port_str;

    int epoll_fd;
    struct epoll_event epoll_events[MAX_EPOLL_EVENTS];
    struct epoll_event server_socket_event;

    int server_socket_fd;
    struct server_socket_event_closure *closure;


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

    closure = malloc(sizeof(struct server_socket_event_closure));
    closure->backend_addr = backend_addr;
    closure->backend_port_str = backend_port_str;
    server_socket_event.data.fd = server_socket_fd;
    server_socket_event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket_fd, &server_socket_event) == -1) {
        perror("Couldn't register server socket with epoll");
        exit(-1);
    }

    while (1) {
        int ii;
        int num_events;

        num_events = epoll_wait(epoll_fd, epoll_events, MAX_EPOLL_EVENTS, -1);
        for (ii=0; ii < num_events; ii++ ) {
            handle_server_socket_event(server_socket_fd, closure);
        }


    }

}

