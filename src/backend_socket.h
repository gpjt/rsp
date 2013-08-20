extern int handle_backend_socket_event(struct epoll_event_handler *self, uint32_t events);

extern struct epoll_event_handler *create_backend_socket_handler(int backend_socket_fd, int client_socket_fd);
