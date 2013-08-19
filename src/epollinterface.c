#include <stdint.h>
#include <sys/epoll.h>

#include "epollinterface.h"

#define MAX_EPOLL_EVENTS 10


void do_reactor_loop(int epoll_fd) {
    struct epoll_event epoll_events[MAX_EPOLL_EVENTS];

    while (1) {
        int ii;
        int num_events;

        num_events = epoll_wait(epoll_fd, epoll_events, MAX_EPOLL_EVENTS, -1);
        for (ii=0; ii < num_events; ii++ ) {
            struct epoll_event_handler_data *handler_data = (struct epoll_event_handler_data *) epoll_events[ii].data.ptr;
            if (handler_data->handle(handler_data, epoll_events[ii].events)) {
                free(handler_data);
            }
        }

    }

}
