#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

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


int main(int argc, char *argv[]) {
    char *server_port_str;
    char *backend_addr;
    char *backend_port_str;

    struct addrinfo hints;
    struct addrinfo *addrs;
    struct addrinfo *addr_iter;
    int getaddrinfo_error;

    int server_socket_fd;
    int client_socket_fd;

    int so_reuseaddr;


    if (argc != 4) {
        fprintf(stderr, 
                "Usage: %s <server_port> <backend_addr> <backend_port>\n", 
                argv[0]);
        exit(1);
    }
    server_port_str = argv[1];
    backend_addr = argv[2];
    backend_port_str = argv[3];

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

    listen(server_socket_fd, MAX_LISTEN_BACKLOG);
    printf("Started.  Listening on port %s.\n", server_port_str);

    while (1) {
        client_socket_fd = accept(server_socket_fd, NULL, NULL);
        if (client_socket_fd == -1) {
            perror("Could not accept");
            exit(1);
        }

        handle_client_connection(client_socket_fd, backend_addr, backend_port_str);
    }

}

