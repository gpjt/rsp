#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include <luajit.h>
#include <lauxlib.h>

#include "epollinterface.h"
#include "server_socket.h"



int main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, 
                "Usage: %s <config_file>\n", 
                argv[0]);
        exit(1);
    }

    lua_State *L = lua_open();
    if (luaL_dofile(L, argv[1]) != 0) {
        fprintf(stderr, "Error parsing config file: %s\n", lua_tostring(L, -1));
        exit(1);
    }

    lua_getglobal(L, "listenPort");
    if (!lua_isstring(L, -1)) {
        fprintf(stderr, "listenPort must be a string");
        exit(1);
    }
    char* server_port_str = (char*) lua_tostring(L, -1);
    printf("sps: %s\n", server_port_str);

    lua_getglobal(L, "backendAddress");
    if (!lua_isstring(L, -1)) {
        fprintf(stderr, "backendAddress must be a string");
        exit(1);
    }
    char* backend_addr = (char*) lua_tostring(L, -1);
    printf("ba: %s\n", backend_addr);

    lua_getglobal(L, "backendPort");
    if (!lua_isstring(L, -1)) {
        fprintf(stderr, "backendPort must be a string");
        exit(1);
    }
    char* backend_port_str = (char*) lua_tostring(L, -1);
    printf("bp: %s\n", backend_port_str);

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Couldn't create epoll FD");
        exit(1);
    }

    struct epoll_event_handler* server_socket_event_handler;
    server_socket_event_handler = create_server_socket_handler(epoll_fd, 
                                                               server_port_str,
                                                               backend_addr,
                                                               backend_port_str);

    add_epoll_handler(epoll_fd, server_socket_event_handler, EPOLLIN);

    printf("Started.  Listening on port %s.\n", server_port_str);
    do_reactor_loop(epoll_fd);

    return 0;
}

