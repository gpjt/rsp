#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include "epollinterface.h"

#define MAX_EPOLL_EVENTS 10


void add_epoll_handler(int epoll_fd, struct epoll_event_handler *handler, uint32_t event_mask) {
    struct epoll_event event;

    event.data.ptr = handler;
    event.events = event_mask;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, handler->fd, &event) == -1) {
        perror("Couldn't register server socket with epoll");
        exit(-1);
    }
}


void do_reactor_loop(int epoll_fd) {
    struct epoll_event epoll_events[MAX_EPOLL_EVENTS];

    while (1) {
        int ii;
        int num_events;

        num_events = epoll_wait(epoll_fd, epoll_events, MAX_EPOLL_EVENTS, -1);
        for (ii=0; ii < num_events; ii++ ) {
            struct epoll_event_handler *handler = (struct epoll_event_handler *) epoll_events[ii].data.ptr;
            if (handler->handle(handler, epoll_events[ii].events)) {
                free(handler);
            }
        }

    }

}
