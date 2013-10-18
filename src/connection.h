struct connection_closure {
    struct epoll_event_handler* backend_handler;
    struct data_buffer_entry* write_buffer;
};

extern void connection_handle_event(struct epoll_event_handler* self, uint32_t events);

extern void connection_write(struct epoll_event_handler* self, char* data, int len);

extern void connection_close(struct epoll_event_handler* self);

extern struct epoll_event_handler* create_connection(int epoll_fd, int connection_fd);
