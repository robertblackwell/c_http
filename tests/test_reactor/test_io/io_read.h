#ifndef c_http_tests_reactor_io_read_h
#define c_http_tests_reactor_io_read_h

#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <sys/epoll.h>
#include <math.h>
#include <c_http/common/utils.h>
#include <c_http/simple_runloop/runloop.h>

typedef struct ReadCtx_s {
    int                 ctx_tag;
    int                 read_count;
    int                 max_read_count;
    char*               id;
    int                 readfd;
    WIoFdRef            swatcher;
} ReadCtx;

typedef struct Reader_s {
    int     rd_tag;
    int     count;
    ReadCtx ctx_table[10];
} Reader;

void Reader_init(Reader* this);
Reader* Reader_new();
void Reader_dispose(Reader* this);
void Reader_add_fd(Reader* this, int fd, int max);

void rd_callback(WIoFdRef socket_watcher_ref, void* arg, uint64_t event);
void* reader_thread_func(void* arg);
#endif