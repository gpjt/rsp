struct backend_socket_event_data {
    int client_socket_fd;
};

extern int handle_backend_socket_event(struct epoll_event_handler_data *self, uint32_t events);
