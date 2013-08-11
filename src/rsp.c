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
    int backend_socket_fd;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    if (getaddrinfo(backend_host, backend_port_str, &hints, &addrs) != 0) {
        perror("Couldn't find backend");
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
}


int main(int argc, char *argv[]) {
    int server_port;
    int server_socket_fd;
    int client_socket_fd;
    struct sockaddr_in server_socket_addr;
    struct sockaddr_in client_socket_addr;
    int client_socket_addr_size;
    char *backend_addr;
    char *backend_port_str;

    if (argc != 4) {
        fprintf(stderr, 
                "Usage: %s <server_port> <backend_addr> <backend_port>\n", 
                argv[0]);
        exit(1);
    }
    server_port = atoi(argv[1]);
    backend_addr = argv[2];
    backend_port_str = argv[3];

    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd < 0) {
        perror("Could not open socket");
        exit(1);
    }

    memset(&server_socket_addr, 0, sizeof(server_socket_addr));
    server_socket_addr.sin_family = AF_INET;
    server_socket_addr.sin_port = htons(server_port);
    server_socket_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket_fd, 
             (struct sockaddr *) &server_socket_addr, 
             sizeof(server_socket_addr)) < 0) 
    {
        perror("Could not bind");
        exit(1);
    }

    listen(server_socket_fd, MAX_LISTEN_BACKLOG);

    client_socket_addr_size = sizeof(client_socket_addr);
    client_socket_fd = accept(server_socket_fd, 
                              (struct sockaddr *) &client_socket_addr, 
                              &client_socket_addr_size);
    if (client_socket_fd < 0) {
        perror("Could not accept");
        exit(1);
    }

    handle_client_connection(client_socket_fd, backend_addr, backend_port_str);
}
