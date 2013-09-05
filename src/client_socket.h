extern void handle_client_socket_event(struct epoll_event_handler* self, uint32_t events);

extern void close_client_socket(struct epoll_event_handler*);

extern struct epoll_event_handler* create_client_socket_handler(int client_socket_fd, int epoll_fd, char* backend_host, char* backend_port_str);
