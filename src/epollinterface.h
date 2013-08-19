struct epoll_event_handler_data {
    int fd;
    int (*handle)(struct epoll_event_handler_data *, uint32_t);
    void *closure;
};
