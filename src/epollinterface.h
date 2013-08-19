struct epoll_event_handler_data {
    int fd;
    int (*handle)(struct epoll_event_handler_data *, uint32_t);
    void *closure;
};

extern void add_epoll_handler(int epoll_fd, struct epoll_event_handler_data *handler, uint32_t event_mask);

extern void do_reactor_loop(int epoll_fd);
