struct epoll_event_handler {
    int fd;
    void (*handle)(struct epoll_event_handler*, uint32_t);
    void* closure;
};

extern void add_epoll_handler(int epoll_fd, struct epoll_event_handler* handler, uint32_t event_mask);

extern void do_reactor_loop(int epoll_fd);
