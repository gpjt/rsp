extern int handle_client_socket_event(struct epoll_event_handler *self, uint32_t events);

extern struct epoll_event_handler *create_client_socket_handler(int client_socket_fd, int backend_socket_fd);
