#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <signal.h>

#include <luajit.h>
#include <lauxlib.h>

#include "epollinterface.h"
#include "logging.h"
#include "server_socket.h"


char* get_config_opt(lua_State* L, char* name) {
    lua_getglobal(L, name);
    if (!lua_isstring(L, -1)) {
        fprintf(stderr, "%s must be a string", name);
        exit(1);
    }
    return (char*) lua_tostring(L, -1);
}


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
    char* server_port_str = get_config_opt(L, "listenPort");
    char* backend_addr = get_config_opt(L, "backendAddress");
    char* backend_port_str = get_config_opt(L, "backendPort");

    signal(SIGPIPE, SIG_IGN);

    epoll_init();

    create_server_socket_handler(server_port_str,
                                 backend_addr,
                                 backend_port_str);

    rsp_log("Started.  Listening on port %s.", server_port_str);
    epoll_do_reactor_loop();

    return 0;
}

