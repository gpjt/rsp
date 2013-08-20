struct client_socket_event_data {
    int backend_socket_fd;
};



extern int handle_client_socket_event(struct epoll_event_handler *self, uint32_t events);
