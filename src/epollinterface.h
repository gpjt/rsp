struct epoll_event_handler_data {
    int fd;
    int (*handle)(struct epoll_event_handler_data *, uint32_t);
    void *closure;
};

extern void do_reactor_loop(int epoll_fd);
