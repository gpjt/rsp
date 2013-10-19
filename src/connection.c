#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <string.h>


#include "connection.h"
#include "epollinterface.h"
#include "netutils.h"


#define BUFFER_SIZE 4096

struct data_buffer_entry {
    int is_close_message;
    char* data;
    int current_offset;
    int len;
    struct data_buffer_entry* next;
};


void connection_really_close(struct epoll_event_handler* self)
{
    struct connection_closure* closure = (struct connection_closure* ) self->closure;
    struct data_buffer_entry* next;
    while (closure->write_buffer != NULL) {
        next = closure->write_buffer->next;
        if (!closure->write_buffer->is_close_message) {
            free(closure->write_buffer->data);
        }
        free(closure->write_buffer);
        closure->write_buffer = next;
    }

    close(self->fd);
    free(self->closure);
    free(self);
}


void connection_on_out_event(struct epoll_event_handler* self)
{
    struct connection_closure* closure = (struct connection_closure*) self->closure;
    int written;
    int to_write;
    struct data_buffer_entry* temp;
    while (closure->write_buffer != NULL) {
        if (closure->write_buffer->is_close_message) {
            connection_really_close(self);
            return;
        }

        to_write = closure->write_buffer->len - closure->write_buffer->current_offset;
        written = write(self->fd, closure->write_buffer->data + closure->write_buffer->current_offset, to_write);
        if (written != to_write) {
            if (written == -1) {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("Error writing to client");
                    exit(-1);
                }
                written = 0;
            }
            closure->write_buffer->current_offset += written;
            break;
        } else {
            temp = closure->write_buffer;
            closure->write_buffer = closure->write_buffer->next;
            free(temp->data);
            free(temp);
        }
    }
}


void connection_on_in_event(struct epoll_event_handler* self)
{
    struct connection_closure* closure = (struct connection_closure*) self->closure;
    char read_buffer[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = read(self->fd, read_buffer, BUFFER_SIZE)) != -1 && bytes_read != 0) {
        if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return;
        }

        if (bytes_read == 0 || bytes_read == -1) {
            connection_on_close_event(self);
            return;
        }

        connection_write(closure->peer, read_buffer, bytes_read);
    }
}


void connection_on_close_event(struct epoll_event_handler* self)
{
    struct connection_closure* closure = (struct connection_closure*) self->closure;
    connection_close(closure->peer);
    connection_close(self);
}


void connection_handle_event(struct epoll_event_handler* self, uint32_t events)
{
    if (events & EPOLLOUT) {
        connection_on_out_event(self);
    }

    if (events & EPOLLIN) {
        connection_on_in_event(self);
    }

    if ((events & EPOLLERR) | (events & EPOLLHUP) | (events & EPOLLRDHUP)) {
        connection_on_close_event(self);
    }

}


void add_write_buffer_entry(struct connection_closure* closure, struct data_buffer_entry* new_entry) 
{
    struct data_buffer_entry* last_buffer_entry;
    if (closure->write_buffer == NULL) {
        closure->write_buffer = new_entry;
    } else {
        for (last_buffer_entry=closure->write_buffer; last_buffer_entry->next != NULL; last_buffer_entry=last_buffer_entry->next)
            ;
        last_buffer_entry->next = new_entry;
    }
}


void connection_write(struct epoll_event_handler* self, char* data, int len)
{
    struct connection_closure* closure = (struct connection_closure* ) self->closure;

    int written = 0;
    if (closure->write_buffer == NULL) {
        written = write(self->fd, data, len);
        if (written == len) {
            return;
        }
    }
    if (written == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Error writing to client");
            exit(-1);
        }
        written = 0;
    }

    int unwritten = len - written;
    struct data_buffer_entry* new_entry = malloc(sizeof(struct data_buffer_entry));
    new_entry->is_close_message = 0;
    new_entry->data = malloc(unwritten);
    memcpy(new_entry->data, data + written, unwritten);
    new_entry->current_offset = 0;
    new_entry->len = unwritten;
    new_entry->next = NULL;

    add_write_buffer_entry(closure, new_entry);
}


void connection_close(struct epoll_event_handler* self)
{
    struct connection_closure* closure = (struct connection_closure* ) self->closure;
    if (closure->write_buffer == NULL) {
        connection_really_close(self);
    } else {
        struct data_buffer_entry* new_entry = malloc(sizeof(struct data_buffer_entry));
        new_entry->is_close_message = 1;
        new_entry->next = NULL;

        add_write_buffer_entry(closure, new_entry);
    }
}


struct epoll_event_handler* create_connection(int epoll_fd, int client_socket_fd)
{
    
    make_socket_non_blocking(client_socket_fd);

    struct connection_closure* closure = malloc(sizeof(struct connection_closure));

    struct epoll_event_handler* result = malloc(sizeof(struct epoll_event_handler));
    result->fd = client_socket_fd;
    result->handle = connection_handle_event;
    result->closure = closure;

    closure->write_buffer = NULL;

    add_epoll_handler(epoll_fd, result, EPOLLIN | EPOLLRDHUP | EPOLLET | EPOLLOUT);

    return result;
}
