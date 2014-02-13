#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <sys/epoll.h>

#include "epollinterface.h"
#include "connection.h"
#include "logging.h"
#include "netutils.h"
#include "server_socket.h"

#define MAX_LISTEN_BACKLOG 4096


struct server_socket_event_data {
    char* backend_addr;
    char* backend_port_str;
};


struct proxy_data {
    struct epoll_event_handler* client;
    struct epoll_event_handler* backend;
};


void on_client_read(void* closure, char* buffer, int len)
{
    struct proxy_data* data = (struct proxy_data*) closure;
    if (data->backend == NULL) {
        return;
    }
    connection_write(data->backend, buffer, len);
}


void on_client_close(void* closure)
{
    struct proxy_data* data = (struct proxy_data*) closure;
    if (data->backend == NULL) {
        return;
    }
    connection_close(data->backend);
    data->client = NULL;
    data->backend = NULL;
    epoll_add_to_free_list(closure);
}


void on_backend_read(void* closure, char* buffer, int len)
{
    struct proxy_data* data = (struct proxy_data*) closure;
    if (data->client == NULL) {
        return;
    }
    connection_write(data->client, buffer, len);
}


void on_backend_close(void* closure)
{
    struct proxy_data* data = (struct proxy_data*) closure;
    if (data->client == NULL) {
        return;
    }
    connection_close(data->client);
    data->client = NULL;
    data->backend = NULL;
    epoll_add_to_free_list(closure);
}


void handle_client_connection(int client_socket_fd, 
                              char* backend_host, 
                              char* backend_port_str) 
{
    struct epoll_event_handler* client_connection;
    rsp_log("Creating connection object for incoming connection...");
    client_connection = create_connection(client_socket_fd);

    int backend_socket_fd = connect_to_backend(backend_host, backend_port_str);
    struct epoll_event_handler* backend_connection;
    rsp_log("Creating connection object for backend connection...");
    backend_connection = create_connection(backend_socket_fd);

    struct proxy_data* proxy = malloc(sizeof(struct proxy_data));
    proxy->client = client_connection;
    proxy->backend = backend_connection;

    struct connection_closure* client_closure = (struct connection_closure*) client_connection->closure;
    client_closure->on_read = on_client_read;
    client_closure->on_read_closure = proxy;
    client_closure->on_close = on_client_close;
    client_closure->on_close_closure = proxy;

    struct connection_closure* backend_closure = (struct connection_closure*) backend_connection->closure;
    backend_closure->on_read = on_backend_read;
    backend_closure->on_read_closure = proxy;
    backend_closure->on_close = on_backend_close;
    backend_closure->on_close_closure = proxy;
}



void handle_server_socket_event(struct epoll_event_handler* self, uint32_t events)
{
    struct server_socket_event_data* closure = (struct server_socket_event_data*) self->closure;

    int client_socket_fd;
    while (1) {
        client_socket_fd = accept(self->fd, NULL, NULL);
        if (client_socket_fd == -1) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                break;
            } else {
                rsp_log_error("Could not accept");
                exit(1);
            }
        }

        handle_client_connection(client_socket_fd,
                                 closure->backend_addr,
                                 closure->backend_port_str);
    }
}


int create_and_bind(char* server_port_str)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* addrs;
    int getaddrinfo_error;
    getaddrinfo_error = getaddrinfo(NULL, server_port_str, &hints, &addrs);
    if (getaddrinfo_error != 0) {
        rsp_log("Couldn't find local host details: %s", gai_strerror(getaddrinfo_error));
        exit(1);
    }

    int server_socket_fd;
    struct addrinfo* addr_iter;
    for (addr_iter = addrs; addr_iter != NULL; addr_iter = addr_iter->ai_next) {
        server_socket_fd = socket(addr_iter->ai_family,
                                  addr_iter->ai_socktype,
                                  addr_iter->ai_protocol);
        if (server_socket_fd == -1) {
            continue;
        }

        int so_reuseaddr = 1;
        if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof(so_reuseaddr)) != 0) {
            continue;
        }

        if (bind(server_socket_fd,
                 addr_iter->ai_addr,
                 addr_iter->ai_addrlen) == 0)
        {
            break;
        }

        close(server_socket_fd);
    }

    if (addr_iter == NULL) {
        rsp_log("Couldn't bind");
        exit(1);
    }

    freeaddrinfo(addrs);

    return server_socket_fd;
}


struct epoll_event_handler* create_server_socket_handler(char* server_port_str,
                                                         char* backend_addr,
                                                         char* backend_port_str)
{

    int server_socket_fd;
    server_socket_fd = create_and_bind(server_port_str);
    make_socket_non_blocking(server_socket_fd);

    listen(server_socket_fd, MAX_LISTEN_BACKLOG);

    struct server_socket_event_data* closure = malloc(sizeof(struct server_socket_event_data));
    closure->backend_addr = backend_addr;
    closure->backend_port_str = backend_port_str;

    struct epoll_event_handler* result = malloc(sizeof(struct epoll_event_handler));
    result->fd = server_socket_fd;
    result->handle = handle_server_socket_event;
    result->closure = closure;

    epoll_add_handler(result, EPOLLIN | EPOLLET);

    return result;
}


